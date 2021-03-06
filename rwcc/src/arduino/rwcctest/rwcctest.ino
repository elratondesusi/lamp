#define RED 9
#define WARM 10
#define COLD1 11
#define COLD2 3
#define SWITCH_RED 5
#define SWITCH_WARM 6
#define SWITCH_COLD1 7
#define SWITCH_COLD2 8

uint8_t r, w, c1, c2;

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
  Serial.print("R=");
  Serial.print(r);
  Serial.print(",W=");
  Serial.print(w);
  Serial.print(",C1=");
  Serial.print(c1);
  Serial.print(",C2=");
  Serial.print(c2);
  Serial.println();
}

void setup() {
  pinMode(RED, OUTPUT);
  pinMode(WARM, OUTPUT);
  pinMode(COLD1, OUTPUT);
  pinMode(COLD2, OUTPUT);
  pinMode(SWITCH_RED, OUTPUT);
  pinMode(SWITCH_WARM, OUTPUT);
  pinMode(SWITCH_COLD1, OUTPUT);
  pinMode(SWITCH_COLD2, OUTPUT);
  digitalWrite(SWITCH_RED, HIGH);
  digitalWrite(SWITCH_WARM, HIGH);
  digitalWrite(SWITCH_COLD1, HIGH);
  digitalWrite(SWITCH_COLD2, HIGH);
  Serial.begin(115200);
  r = 0;
  w = 0; 
  c1 = 0;
  c2 = 0;
}

void loop() 
{
  if (Serial.available())
  {
    char c = Serial.read();
    if (c == 'a') 
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
  }
}
