#define RED 9
#define WARM 10
#define COLD1 11
#define COLD2 3
#define SWITCH_RED 5
#define SWITCH_WARM 6
#define SWITCH_COLD1 7
#define SWITCH_COLD2 8

#define SIRENE 12
#define RESET_SIGNAL 4

// wire connections on model #001
//
// 3 - red (cold2 pwm)
// 5 - green (red relay)
// 6 - black (warm relay)
// 7 - orange (cold1 relay)
// 8 - brown (cold2 relay)
// 9 - white (red pwm)
// 10 - yellow (warm pwm)
// 11 - blue (cold1 pwm)

uint8_t r, w, c1, c2;

uint8_t switch_red, switch_warm, switch_cold1, switch_cold2;
uint8_t bak_red, bak_warm, bak_cold1, bak_cold2;

uint32_t time_break_started;
uint32_t max_break_time_in_minutes;
uint8_t toilet_break;

uint8_t last_reset_signal;
uint32_t last_time_reset_signal_changed;

void switch_if_needed(uint8_t val, uint8_t newval, uint8_t pin_number)
{
  if ((val > 0) && (newval == 0))
    digitalWrite(pin_number, HIGH);
  else if ((val == 0) && (newval > 0))
    digitalWrite(pin_number, LOW);
}

void setColor(uint8_t newr, uint8_t neww, uint8_t newc1, uint8_t newc2)
{
  switch_if_needed(r, newr, SWITCH_RED);
  switch_if_needed(w, neww, SWITCH_WARM);
  switch_if_needed(c1, newc1, SWITCH_COLD1); 
  switch_if_needed(c2, newc2, SWITCH_COLD2); 
  if (r != newr)
  {
    r = newr;
    analogWrite(RED, r);
  }
  if (w != neww)
  {
    w = neww;
    analogWrite(WARM, w);
  }
  if (c1 != newc1)
  {
    c1 = newc1;
    analogWrite(COLD1, c1);
  }  
  if (c2 != newc2)
  {
    c2 = newc2;
    analogWrite(COLD2, c2);
  }
}

void setup() 
{
  pinMode(RED, OUTPUT);
  pinMode(WARM, OUTPUT);
  pinMode(COLD1, OUTPUT);
  pinMode(COLD2, OUTPUT);
  pinMode(SWITCH_RED, OUTPUT);
  pinMode(SWITCH_WARM, OUTPUT);
  pinMode(SWITCH_COLD1, OUTPUT);
  pinMode(SWITCH_COLD2, OUTPUT);
  pinMode(SIRENE, OUTPUT);
  pinMode(RESET_SIGNAL, INPUT_PULLUP);
  digitalWrite(SWITCH_RED, HIGH);
  digitalWrite(SWITCH_WARM, HIGH);
  digitalWrite(SWITCH_COLD1, HIGH);
  digitalWrite(SWITCH_COLD2, HIGH);
  Serial.begin(115200);
  r = 0;
  w = 0; 
  c1 = 0;
  c2 = 0;
  switch_red = 200; switch_warm = 0; switch_cold1 = 0; switch_cold2 = 0;
  max_break_time_in_minutes = 5;
  toilet_break = 0;

  delay(50);
  last_reset_signal = digitalRead(RESET_SIGNAL);
  last_time_reset_signal_changed = millis();
}

char readchar()
{
  while (!Serial.available());
  return Serial.read();
}

uint8_t readbyte()
{
  while (!Serial.available());
  return Serial.read();  
}

void toilet_break_start()
{
  bak_red = r; bak_warm = w; bak_cold1 = c1; bak_cold2 = c2;
  setColor(switch_red, switch_warm, switch_cold1, switch_cold2);
  time_break_started = millis();
  toilet_break = 1;
}

void toilet_break_stop()
{
  setColor(bak_red, bak_warm, bak_cold1, bak_cold2);
  toilet_break = 0;
}

void reset_beep()
{
  tone(SIRENE, 1760, 50);
  delay(100);
  tone(SIRENE, 1760, 50);
  delay(100);
}    

void send_reset_signal()
{
  Serial.write("*****");  
}

void half_beep()
{
  tone(SIRENE, 880, 50);
  delay(100);
}

void loop() 
{
  if (toilet_break)
    if (millis() - time_break_started > max_break_time_in_minutes * 60L * 1000L)
      toilet_break_stop();

  uint8_t new_signal = digitalRead(RESET_SIGNAL);
  if ((new_signal != last_reset_signal) && (millis() - last_time_reset_signal_changed > 200))
  {
    last_reset_signal ^= 1;
    if (last_reset_signal) 
    {
      reset_beep();
      send_reset_signal();
    }
    else half_beep();
    last_time_reset_signal_changed = millis();
  }
  if (Serial.available())
  {
    char c = Serial.read();
    if (c == 'C')
    {
      uint8_t day = readbyte();
      uint8_t red = readbyte();
      uint8_t warm = readbyte();
      uint8_t cold1 = readbyte();
      uint8_t cold2 = readbyte();
      if (day == 1)
      {
        if (toilet_break) 
        {
          bak_red = red;
          bak_warm = warm;
          bak_cold1 = cold1;
          bak_cold2 = cold2;
        }
        else setColor(red, warm, cold1, cold2);
      }
      else 
      {
        switch_red = red;
        switch_warm = warm;
        switch_cold1 = cold1;
        switch_cold2 = cold2;
      }
    }
    else if (c == 'B') toilet_break_start();
    else if (c == 'R') toilet_break_stop();
    else if (c == 'M')
    {
      max_break_time_in_minutes = readchar();
    }
    else if (c == 'a') 
    {
      if (r < 255) setColor(r + 1, w, c1, c2);   
    }
    else if (c == 'z')
    {
      if (r > 0) setColor(r - 1, w, c1, c2);
    }
    else if (c == 's')
    {
      if (w < 255) setColor(r, w + 1, c1, c2);    
    }
    else if (c == 'x')
    {
      if (w > 0) setColor(r, w - 1, c1, c2);
    }
    else if (c == 'd')
    {
      if (c1 < 255) setColor(r, w, c1 + 1, c2);
    }
    else if (c == 'c')
    {
      if (c1 > 0) setColor(r, w, c1 - 1, c2);
    }
    else if (c == 'f')
    {
      if (c2 < 255) setColor(r, w, c1, c2 + 1);
    }
    else if (c == 'v')
    {
      if (c2 > 0) setColor(r, w, c1, c2 - 1);
    }
    else if (c == 'S')
    {      
      if (readchar() == 'H')
      if (readchar() == 'A') 
      if (readchar() == 'K') 
      if (readchar() == 'E') 
      if (readchar() == '?')
      {
         reset_beep();
         Serial.print("SHAKE!");
      }
    }
    else if (c == 'T')
    {
      int n = readchar();
      switch (n) {
        case 1: tone(SIRENE, 440, 50); delay(100); break;
        case 2: tone(SIRENE, 262, 50); delay(100); break;
        case 3: tone(SIRENE, 330, 50); delay(100); break;
        case 4: tone(SIRENE, 220, 50); delay(100); break;
      }
    }
    else if (c == '@')
      Serial.print('#');
  }
}
