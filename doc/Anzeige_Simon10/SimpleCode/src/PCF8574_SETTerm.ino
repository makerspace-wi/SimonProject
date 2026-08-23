#include <Arduino.h>
#include <Wire.h>
 
const uint8_t PCF_ADDR = 0x38;
int val = 0;
int value = 0;
String inStr = "";      // a string to hold incoming data

// FUNCTIONS ------------------------------------
int getNum(String strNum) // Check if realy numbers
{
  strNum.trim();
  for (byte i = 0; i < strNum.length(); i++)
  {
    if (!isDigit(strNum[i])) 
    {
      Serial.println(F("Error: no number from 0 - 255"));
      return value;
    }
  }
  return strNum.toInt();
}
// End Funktions --------------------------------
 
// Funktions Serial Input (Event) ---------------
void evalSerialData()
{
  if (inStr.length() <4)
  { 
    val = getNum(inStr);
    if (val > 255) 
    {
      Serial.println(F("Error: > 255"));
    }
    else  
    {
      value = val;
    }   
    Serial.print(F("Value:"));
  }
  else
  {
    Serial.println(F("Error: max 3 digits"));
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
  Wire.beginTransmission(PCF_ADDR);
  Wire.write(value);
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
  Wire.beginTransmission(PCF_ADDR);
  Wire.write(value);  // value set the output
  Wire.endTransmission();
  
  delay(50);

}
