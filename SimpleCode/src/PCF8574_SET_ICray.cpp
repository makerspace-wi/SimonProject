#include <Arduino.h>
#include <Wire.h>
 
#define Ziffer1 0x38
#define Ziffer2 0x39
#define Ziffer3 0x3A
int val = 0;
int valZ1 = 255; // initial value for all ports high (inputs/high-Z)
int valZ2 = 255; // initial value for all ports high (inputs/high-Z)
int valZ3 = 255; // initial value for all ports high (inputs/high-Z)
int maxValue = 63; // max value for 6 bit output
String inStr = ""; // a string to hold incoming data

// FUNCTIONS ------------------------------------
int getNum(String strNum) // Check if realy numbers
{
  strNum.trim();
  for (byte i = 0; i < strNum.length(); i++)
  {
    if (!isDigit(strNum[i])) 
    {
      Serial.println("Error: no number from 0 - " + String(maxValue));
      return valZ1;
    }
  }
  return strNum.toInt();
}

int bin2gray(byte n)
{
  return 0b11111111 & ~(n ^ (n >> 1));
}
// End Funktions --------------------------------
 
// Funktions Serial Input (Event) ---------------
void evalSerialData()
{
  if (inStr.length() <4)
  { 
    val = getNum(inStr);
    if (val > maxValue) 
    {
      Serial.println("Error: > " + String(maxValue));
    }
    else  
    {
      valZ1 = bin2gray(val);
      valZ2 = bin2gray(val);
      valZ3 = bin2gray(val);
    }
    Serial.println("Value: " + String(val) + " -> InvGray: " + String(valZ1));
    Serial.print(F("Value:"));
  }
  else
  {
    Serial.println(F("Error: max 2 digits"));
    Serial.print(F("Value:"));
  }
}
// End Funktions Serial Input -------------------

void setup()
{
  Wire.begin(); 
  Serial.begin(115200);
  inStr.reserve(10);    // reserve for instr serial input

  // Initialize all ports high (inputs/high-Z)
  Wire.beginTransmission(Ziffer1);
  Wire.write(valZ1);
  Wire.endTransmission();
  Wire.beginTransmission(Ziffer2);
  Wire.write(valZ2);
  Wire.endTransmission(); 
  Wire.beginTransmission(Ziffer3);
  Wire.write(valZ3);
  Wire.endTransmission();
  Serial.print(F("Value:"));
}
 
// PROGRAM LOOP AREA ----------------------------
void loop()
{
  if (Serial.available() > 0)
  {
    char inChar = (char)Serial.read();
    if (inChar == '\x0d')
    {
      evalSerialData();
      inStr = "";
    }
    else if (inChar != '\x0a')
    {
      inStr += inChar;
    }
  }
  // Set test value AA
  Wire.beginTransmission(Ziffer1);
  Wire.write(valZ1);  // value set the output
  Wire.endTransmission();
  delay(50);
  Wire.beginTransmission(Ziffer2);
  Wire.write(valZ2);  // value set the output
  Wire.endTransmission();
  delay(50);
  Wire.beginTransmission(Ziffer3);
  Wire.write(valZ3);  // value set the output
  Wire.endTransmission();
}
