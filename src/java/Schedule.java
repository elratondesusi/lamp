import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public class Schedule {
    Map<String,Mode> modes;
    List<TimeAndMode> initialModes;
    Schedule(){
        initialModes = new ArrayList<>();
        modes = new LinkedHashMap<>();
    }
}

