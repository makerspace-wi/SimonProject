#include <Arduino.h>

#define Message "Hello, RS-485?"

static const int RXD2 = 16;
static const int TXD2 = 17;
static const int RS485_DE_RE = 4;

void preTransmission() {
  digitalWrite(RS485_DE_RE, HIGH);
}

void postTransmission() {
  digitalWrite(RS485_DE_RE, LOW);
}

void setup() {
  pinMode(RS485_DE_RE, OUTPUT);
  digitalWrite(RS485_DE_RE, LOW);

  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
}

void loop() {
  // if (Serial2.available()) {
  //   char data = Serial2.read();  // read the received character
  //   Serial.print(data);          // print the recived data to Serial Monitor
  // }
  digitalWrite(RS485_DE_RE, HIGH);
  Serial2.println(String(Message));  // send data over RS-485
  delay(100);
  Serial.println("Send:" + String(Message));
  digitalWrite(RS485_DE_RE, LOW);
}