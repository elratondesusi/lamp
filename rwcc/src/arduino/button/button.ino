#include <SoftwareSerial.h>

SoftwareSerial ss(2, 4);
uint8_t button;
uint32_t last_change;

void setup() 
{
  pinMode(12, INPUT);
  Serial.begin(115200);
  ss.begin(9600);
  button = digitalRead(12);
  last_change = millis();
}

char readchar()
{
  while (!ss.available());
  return ss.read();
}

uint8_t readbyte()
{
  while (!ss.available());
  return ss.read();  
}

void loop() 
{
  if (ss.available())
  {
    char c = ss.read();
    if (c == 'S')
    {      
      if (readchar() == 'H')
      if (readchar() == 'A') 
      if (readchar() == 'K') 
      if (readchar() == 'E') 
      if (readchar() == '?')
         ss.print("SHAKE!");
    }
    else if (c == '@') ss.print("!");
  }
  if ((digitalRead(12) != button) && (millis() - last_change > 500))
  {
    button ^= 1;
    last_change = millis();
    ss.print(button);
    ss.flush();
    Serial.print(button);
  }
}
