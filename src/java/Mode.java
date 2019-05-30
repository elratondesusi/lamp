public class Mode {
     int color1 =0xffffff;
     int color2=0xffffff;
     TimeAndMode at;
     TimeAndMode after;

     String alarm;
     String turn;
     String touch;
	 String loop3;
     int id = -1;
     Mode(){
         at = new TimeAndMode();
         after = new TimeAndMode();
     }
}
class TimeAndMode {
    int tm[];
    String mode;
    TimeAndMode(){ tm = new int[3];}
}
