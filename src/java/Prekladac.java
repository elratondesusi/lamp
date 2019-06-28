import java.io.*;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.TreeMap;

public class Prekladac {
    static int  ids = 0;
	static int earlyAlarm = 0;
	static String settingX = "0";
	
    static TimeAndMode parseTimeAndMode(String line, int from, String arrayName, int arrayIndex) throws Exception {
        TimeAndMode tm = new TimeAndMode();
        String [] values = line.substring(from).split(" ");
        if (values.length < 2)
            throw new Exception("Can not parse " + line);
        tm.mode = values[1];
        String [] time = values[0].split(":");
        if (time.length == 3)
		{
			for (int i = 0; i < 3; i++) {
				tm.tm[i] = Integer.parseInt(time[i]);
			}
		}
        else if (time.length == 1)
		{
			tm.tm[0] = 0;
			try { tm.tm[1] = Integer.parseInt(time[0]); }
			catch (NumberFormatException ex)
			{
				tm.tm[1] = 0;
				if (time[0].equals("X")) 
					settingX = "&(" + arrayName + "[" + arrayIndex + "][1])" ;
			}
			tm.tm[2] = 0;
		}
		else 
		{
            throw new Exception("Can not parse " + line);
		}
		return tm;
    }
    public static Mode readMode(BufferedReader br, String name) throws  Exception {
            Mode m = new Mode();
            m.id = ids;
            ids++;
        while (true) {
            String line = br.readLine();
            if(line == null)
                break;
            if(line.trim().length() == 0)
                break;
            if (line.substring(0, 8).equals("COLOR1=#")) {
                m.color1 = Integer.parseInt(line.substring(8), 16);
            } else if (line.substring(0, 8).equals("COLOR2=#")) {
                m.color2 = Integer.parseInt(line.substring(8), 16);
            } else if (line.substring(0, 3).equals("AT ")) {
                m.at = parseTimeAndMode(line, 3, "atMode", m.id);
            } else if (line.substring(0, 6).equals("AFTER ")) {
                m.after = parseTimeAndMode(line, 6, "afterMode", m.id);
            } else if (line.substring(0, 6).equals("ALARM ")) {
                m.alarm = line.substring(6).trim();
            } else if (line.substring(0, 5).equals("TURN ")) {
                m.turn = line.substring(5).trim();
            } else if (line.substring(0, 6).equals("TOUCH ")) {
                m.touch = line.substring(6).trim();
            } else if (line.substring(0, 6).equals("LOOP3 ")) {
                m.loop3 = line.substring(6).trim();
			}
		}
        return  m;
    }
    static Schedule readModes(String fileName)  throws Exception {
        BufferedReader br = new BufferedReader(new FileReader(fileName));
        int ln = 0;
        Schedule schedule = new Schedule();
       // Map<String,Mode> modes = new LinkedHashMap<>();
	    String l = br.readLine();
		if (l.substring(0, 11).equals("EARLYALARM=")) {
            earlyAlarm = Integer.parseInt(l.substring(11));
		} else {
			throw new Exception("Expecting early alarm, got: " + l);
		}
	    l = br.readLine();
        if(l.trim().length() != 0)
		{
			throw new Exception("Expecting empty line after early alarm, got: " + l);
		}
		
		while(true){
            String line = br.readLine();
            ln++;
            if(line == null)
                break;
            if(line.trim().length() == 0)
                break;
            System.out.println(line);
            TimeAndMode tm = parseTimeAndMode(line, 0, "initalTimes", schedule.initialModes.size());
            schedule.initialModes.add(tm);
        }
        while(true){
            String line = br.readLine();
            ln++;
            if(line == null)
                break;
            if(line.trim().length() == 0)
                continue;
            if(!line.contains("]"))
                throw new Exception("Error format at line " + ln + ": " + line);
            String name = line.substring(1,line.indexOf(']'));
            Mode m = readMode(br, name);
            schedule.modes.put(name, m);
        }
        return schedule;
    }
    static void printThreeValues(PrintWriter pw, int c){
        pw.print("{ ");
        printOnlyThreeValues(pw, c);
        pw.print(" }");
    }
    static void printOnlyThreeValues(PrintWriter pw, int c){
        pw.print((c >> 16 )+ ", ");
        pw.print(((c >> 8) & 255 )+ ", ");
        pw.print((c & 255 ));
    }
    static void printSource(String fileName, Schedule schedule) throws  Exception{
        PrintWriter pw = new PrintWriter(fileName);
        pw.println("#include <Arduino.h>");
	
		pw.println("uint8_t earlyAlarm = " + earlyAlarm + ";");
        pw.println("uint8_t initialTimesCnt = " + schedule.initialModes.size() + ";");

        pw.print("uint8_t initialTimes  [][4] = { ");
        int count = schedule.initialModes.size();
        for(TimeAndMode tm : schedule.initialModes){
            String initMode = tm.mode;
            int initModeNum = -1;
            if(initMode != null) {
                if(schedule.modes.get(initMode) == null)
                    throw new Exception("Unknown mode " + initMode);
                initModeNum = schedule.modes.get(initMode).id;
            }
            printFourValues(pw, tm.tm,initModeNum);
            count--;
            if(count > 0)
                pw.println(", ");
        }
        pw.println(" };");

        pw.print("uint8_t color1  [][3] = { ");
        count = schedule.modes.size();
        for(Map.Entry<String, Mode> i : schedule.modes.entrySet()){
            printThreeValues(pw, i.getValue().color1);
            count--;
            if(count > 0)
                pw.println(", ");
        }
        pw.println(" };");

        count = schedule.modes.size();
        pw.print("uint8_t color2[][3]  = { ");
        for(Map.Entry<String, Mode> i : schedule.modes.entrySet()){
            printThreeValues(pw, i.getValue().color2);
            count--;
            if(count > 0)
                pw.println(", ");
        }
        pw.println(" };");

        count = schedule.modes.size();
        pw.print("int8_t atMode [][4] = { ");
        for(Map.Entry<String, Mode> i : schedule.modes.entrySet()){
            String atMode = i.getValue().at.mode;
            int atModeNum = -1;
            if(atMode != null) {
                if(schedule.modes.get(atMode) == null)
                    throw new Exception("Unknown mode " + atMode);
                atModeNum = schedule.modes.get(atMode).id;
            }
            printFourValues(pw, i.getValue().at.tm,atModeNum);
            count--;
            if(count > 0)
                pw.println(", ");
        }
        pw.println(" };");

        count = schedule.modes.size();
        pw.print("int8_t afterMode [][4] = { ");
        for(Map.Entry<String, Mode> i : schedule.modes.entrySet()){
            String afterMode = i.getValue().after.mode;
            int afterModeNum = -1;
            if(afterMode != null) {
                if (schedule.modes.get(afterMode) == null)
                    throw new Exception("Unknown mode " + afterMode);
                afterModeNum = schedule.modes.get(afterMode).id;
            }
            printFourValues(pw, i.getValue().after.tm,afterModeNum);
            count--;
            if(count > 0)
                pw.println(", ");
        }
        pw.println(" };");

        count = schedule.modes.size();
        pw.print("int8_t alarm [] = { ");
        for(Map.Entry<String, Mode> i : schedule.modes.entrySet()){
            if(i.getValue().alarm != null)
                pw.print(schedule.modes.get(i.getValue().alarm).id);
            else
            pw.print(-1);
            count--;
            if(count > 0)
                pw.println(", ");
        }
        pw.println(" };");

        count = schedule.modes.size();
        pw.print("int8_t turn [] = { ");
        for(Map.Entry<String, Mode> i :schedule.modes.entrySet()){
            if(i.getValue().turn != null)
                pw.print(schedule.modes.get(i.getValue().turn).id);
            else
                pw.print(-1);
            count--;
            if(count > 0)
                pw.println(", ");
        }
        pw.println(" };");

        count = schedule.modes.size();
        pw.print("int8_t touch [] = { ");
        for(Map.Entry<String, Mode> i : schedule.modes.entrySet()){
            if(i.getValue().touch != null)
                pw.print(schedule.modes.get(i.getValue().touch).id);
            else
                pw.print(-1);
            count--;
            if(count > 0)
                pw.println(", ");
        }
        pw.println(" };");
		
		count = schedule.modes.size();
        pw.print("int8_t loop3 [] = { ");
        for(Map.Entry<String, Mode> i : schedule.modes.entrySet()){
            if(i.getValue().loop3 != null)
                pw.print(schedule.modes.get(i.getValue().loop3).id);
            else
                pw.print(-1);
            count--;
            if(count > 0)
                pw.println(", ");
        }
        pw.println(" };");
        pw.println("int8_t *settingX = " + settingX + ";");
        
        pw.close();
    }
    static void printFourValues(PrintWriter pw, int [] vals, int modeID){
        pw.print("{ " );
        for (int j = 0; j < 3; j++) {
            pw.print(vals[j] + ", ");
        }
        pw.print(modeID + " }");
    }
	
	public static String selectFile() throws Exception {
		File folder = new File("schedules");
		File[] listOfFiles = folder.listFiles();

		for (int i = 0; i < listOfFiles.length; i++) {
		  if (listOfFiles[i].isFile()) {
			System.out.println((i+1) + ". " + listOfFiles[i].getName());
		  } 
	    }
		
		int num = 0;
		do {
			BufferedReader br = new BufferedReader(new InputStreamReader(System.in));
			num = Integer.parseInt(br.readLine());
		} while ((num < 1) || (num > listOfFiles.length));
		num--;
		return "schedules\\" + listOfFiles[num].getName();
	}
	
    public static void main(String[] args) throws Exception{
		
        Schedule schedule = readModes(selectFile());
          printSource("src\\arduino\\Lamp\\modes.cpp", schedule);
    }
}
