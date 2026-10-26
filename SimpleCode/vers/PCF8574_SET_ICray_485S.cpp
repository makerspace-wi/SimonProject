#include <Arduino.h>
// Serial2 for RS-485 communication
#define RXD2 16
#define TXD2 17
#define RS485_DE_RE 4
//  Output Pins
#define Bit0 19
#define Bit1 18
#define Bit2 5

#include <Wire.h>
 
const uint8_t PCF_ADDR = 0x3A;
int val = 0;
int value = 255; 		// initial value for all ports high (inputs/high-Z)
int maxValue = 63; 		// max value for 6 bit output
String inStr = "";      // a string to hold incoming data

// FUNCTIONS ------------------------------------
int getNum(String strNum) // Check if realy numbers
{
  strNum.trim();
  for (byte i = 0; i < strNum.length(); i++)
  {
    if (!isDigit(strNum[i])) 
    {
      Serial.println("Error: no number from 0 - " + String(maxValue));
      return value;
    }
  }
  return strNum.toInt();
}

void preTransmission() {
  digitalWrite(RS485_DE_RE, HIGH);
}

void postTransmission() {
  digitalWrite(RS485_DE_RE, LOW);
}

int bin2gray(byte n)
{
  return 0b11111111 & ~(n ^ (n >> 1));
}
// End Funktions --------------------------------
 
// Funktions Serial Input (Event) ---------------
void evalSerialData()
{
  inStr.toUpperCase();
  Serial.println(inStr);          // print the recived data to Serial Monitor
  if (inStr.startsWith("S") && inStr.length() == 2)
  {
    val = getNum(inStr);
    if (val < 8)
    {
      preTransmission();
      Serial2.println(inStr);  // send data over RS-485
      delay(100);
      postTransmission();
    }
    else
    {
      Serial.println(F("Only max 7"));
    }
    return;
	
  }
  else if (inStr.length() <4)
  { 
    val = getNum(inStr);
    if (val > maxValue) 
    {
      Serial.println("Error: > " + String(maxValue));
    }
    else  
    {
      value = bin2gray(val);
    }
    Serial.println("Value: " + String(val) + " -> InvGray: " + String(value));
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
  Serial.begin(115200);
  inStr.reserve(10);    // reserve for instr serial input

  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  Wire.begin(); 
  // Initialize all ports high (inputs/high-Z)
  pinMode(RS485_DE_RE, OUTPUT);
  digitalWrite(RS485_DE_RE, LOW);

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
