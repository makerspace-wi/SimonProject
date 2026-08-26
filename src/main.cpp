#include <Arduino.h>
#include <Wire.h>
// Alex Kohl 2024 Krone Fallblattansteuerung mit ESP32 Email an mich: alex@alex14.de
// ESP32 als Wifi-AP zur Ansteuerung von MAN/Krone/Vossloh Fallblattanzeigen
// Module sind ueber eine serielle Verbindung angebunden
// Aufbau Message: [88] [Adresse] [Blatt] [81]
//  ZUM PROGRAMMIEREN: ESP32 UMBEDINGT von Adapter AUSSTECKEN!

// Einstellungen zum Programmieren:
// 1. PlatformIO-Umgebung: ESP32_WROOM
// 2. Board: wemos_d1_uno32
// 3. Framework: arduino
// >>>> Wenn unten orange Fehlermeldungen kommen, stimmt etwas nicht.
//      Meistens hilft Google oder mich einfach fragen.

// Sonderzeichen zu "Ersatzzeichen": https://www-user.tu-chemnitz.de/~lfe/selfhtml61/tcad.htm

// https://github.com/espressif/arduino-esp32/tree/master/libraries/WebServer
#include <ESP32WebServer.h>
const char *WiFiName = "fallblatt.local";
const char *WiFiPass = "Fallblattanzeige";
// https://github.com/espressif/arduino-esp32/tree/master/libraries/ESPmDNS
#include <ESPmDNS.h>

// web.h: ausgelagertes Webdokument, enthaelt das gesamte HTML
#include "web.h"

int address = 0;
int verschubzahl = 0;
ESP32WebServer server(80);

int laststatus = 3;

void returnConfigPage();
void clear();
void flip();
void setflip(byte adress, byte flip);
void sendByte(byte i2cAddress, byte zahl);
byte bin2grayi(byte n);

void module2SendCommand(uint8_t command, uint8_t address, const uint8_t *payload, size_t payloadLen);
void module2SetPosition(uint8_t address, uint8_t position);
void updateModule2Direct();

#ifndef UART_DEBUG_BAUD
#define UART_DEBUG_BAUD 115200
#endif

#ifndef UART_SIGNAL_BAUD
#define UART_SIGNAL_BAUD 4800
#endif

#ifndef UART_SIGNAL_RX_PIN
#define UART_SIGNAL_RX_PIN 16
#endif

#ifndef UART_SIGNAL_TX_PIN
#define UART_SIGNAL_TX_PIN 17
#endif

#ifndef UART_MODULE2_BAUD
#define UART_MODULE2_BAUD 19200
#endif

#ifndef UART_MODULE2_RX_PIN
#define UART_MODULE2_RX_PIN 27
#endif

#ifndef UART_MODULE2_TX_PIN
#define UART_MODULE2_TX_PIN 26
#endif

#ifndef UART_MODULE2_BREAK_BITS
#define UART_MODULE2_BREAK_BITS 24
#endif

#ifndef UART_MODULE2_RS485_DE_PIN
#define UART_MODULE2_RS485_DE_PIN -1
#endif

#ifndef MODULE2_DIRECT_ENABLED
#define MODULE2_DIRECT_ENABLED 1
#endif

#ifndef MODULE2_DIRECT_ADDR
#define MODULE2_DIRECT_ADDR 0x0A
#endif

#ifndef I2C_ADDR_1
#define I2C_ADDR_1 0x10
#endif

#ifndef I2C_ADDR_2
#define I2C_ADDR_2 0x11
#endif

#ifndef I2C_ADDR_3
#define I2C_ADDR_3 0x12
#endif

#ifndef I2C_SDA_PIN
#define I2C_SDA_PIN -1
#endif

#ifndef I2C_SCL_PIN
#define I2C_SCL_PIN -1
#endif

// I2C-Zieladressen fuer Integer-Transfers
const uint8_t i2cAddress1 = static_cast<uint8_t>(I2C_ADDR_1);
const uint8_t i2cAddress2 = static_cast<uint8_t>(I2C_ADDR_2);
const uint8_t i2cAddress3 = static_cast<uint8_t>(I2C_ADDR_3);

// Adressen der Fallblattmodule
byte logoadress = 0x05;
byte gattungadress = 0x04;
byte char1 = 0x03;
byte char2 = 0x02;

byte stdadress = 0x01;
byte minadress = 0x00;

byte infoadress = 0x07;
byte infoadress2 = 0x06;
byte zieladress = 0x09;

byte zwischenzieladress = 0x08;

// Uebergebene Variablen
int betreiber, gattung, abmin, abstd, zugnummer, info1, info2, zwischenziel, ziel;
String s_betreiber, s_gattung, s_abmin, s_abstd, s_zugnummer, s_info1, s_info2, s_zwischenziel, s_ziel;

void setup()
{
  Serial.begin(UART_DEBUG_BAUD);
  Serial2.begin(UART_SIGNAL_BAUD, SERIAL_8E2, UART_SIGNAL_RX_PIN, UART_SIGNAL_TX_PIN);
  Serial1.begin(UART_MODULE2_BAUD, SERIAL_8N1, UART_MODULE2_RX_PIN, UART_MODULE2_TX_PIN);
  Serial.println("Debug-UART aktiv (Serial)");
  Serial.println("Signal-UART aktiv (Serial2)");
  Serial.println("Modul-2-UART aktiv (Serial1, BREAK + FF Cx Protokoll)");

#if UART_MODULE2_RS485_DE_PIN >= 0
  pinMode(UART_MODULE2_RS485_DE_PIN, OUTPUT);
  digitalWrite(UART_MODULE2_RS485_DE_PIN, LOW);
#endif

#if (I2C_SDA_PIN >= 0) && (I2C_SCL_PIN >= 0)
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
#else
  Wire.begin();
#endif
  delay(100);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WiFiName, WiFiPass);
  WiFi.softAPIP();
  if (!MDNS.begin("fallblatt"))
  {
    Serial.println("Error setting up MDNS responder!");
    while (1)
    {
      delay(1000);
    }
  }

  // Serial.println(WiFi.localIP());

  server.on("/", returnConfigPage);

  server.on("/clear", clear);
  server.on("/flip", flip);
  server.begin();
}

void loop()
{
  server.handleClient();
#if defined(ARDUINO_ARCH_ESP8266)
  MDNS.update();
#endif
}

// Bereitet einkommende Daten auf und leitet entsprechendes Blaettern ein
void flip()
{
  s_betreiber = server.arg("b");
  int str_len1 = s_betreiber.length() + 1;
  char wertigkeit1[str_len1];
  s_betreiber.toCharArray(wertigkeit1, str_len1);
  betreiber = atoi(wertigkeit1);

  s_gattung = server.arg("g");
  int str_len2 = s_gattung.length() + 1;
  char wertigkeit2[str_len2];
  s_gattung.toCharArray(wertigkeit2, str_len2);
  gattung = atoi(wertigkeit2);

  s_zugnummer = server.arg("n");
  int str_len3 = s_zugnummer.length() + 1;
  char wertigkeit3[str_len3];
  s_zugnummer.toCharArray(wertigkeit3, str_len3);
  zugnummer = atoi(wertigkeit3);

  s_abstd = server.arg("s");
  int str_len4 = s_abstd.length() + 1;
  char wertigkeit4[str_len4];
  s_abstd.toCharArray(wertigkeit4, str_len4);
  abstd = atoi(wertigkeit4);

  s_abmin = server.arg("m");
  int str_len5 = s_abmin.length() + 1;
  char wertigkeit5[str_len5];
  s_abmin.toCharArray(wertigkeit5, str_len5);
  abmin = atoi(wertigkeit5);

  s_info1 = server.arg("i1");
  int str_len6 = s_info1.length() + 1;
  char wertigkeit6[str_len6];
  s_info1.toCharArray(wertigkeit6, str_len6);
  info1 = atoi(wertigkeit6);

  s_info2 = server.arg("i2");
  int str_len7 = s_info1.length() + 1;
  char wertigkeit7[str_len7];
  s_info2.toCharArray(wertigkeit7, str_len7);
  info2 = atoi(wertigkeit7);

  s_zwischenziel = server.arg("d");
  int str_len8 = s_zwischenziel.length() + 1;
  char wertigkeit8[str_len8];
  s_zwischenziel.toCharArray(wertigkeit8, str_len8);
  zwischenziel = atoi(wertigkeit8);

  s_ziel = server.arg("z");
  int str_len9 = s_ziel.length() + 1;
  char wertigkeit9[str_len9];
  s_ziel.toCharArray(wertigkeit9, str_len9);
  ziel = atoi(wertigkeit9);

  // DEBUG-Ausgabe
Serial.println("Betreiber: " + String(betreiber) + " Gattung: " + String(gattung) + " Zugnummer: " + String(zugnummer) + " Abfahrt: " + String(abstd) + ":" + String(abmin) + " Info1: " + String(info1) + " Info2: " + String(info2) + " Zwischenziel: " + String(zwischenziel) + " Ziel: " + String(ziel));

  // Zugnummer berechnen
  int len = String(abs(zugnummer)).length();
  byte einer = 0;
  byte zehner = 0;
  byte hunderter = 0;
  byte tausender = 0;
  byte ztausender = 0;

  if (zugnummer == 0)
  {
    // Clear Zugnummeranzeige
    setflip(char2, 35);
    setflip(char1, 35);
    sendByte(i2cAddress1, 0xFF);
    sendByte(i2cAddress2, 0xFF);
    sendByte(i2cAddress3, 0xFF);
  }
  else
  {
    einer = zugnummer % 10;
    zehner = (zugnummer / 10) % 10;
    hunderter = (zugnummer / 100) % 10;
    tausender = (zugnummer / 1000) % 10;
    ztausender = (zugnummer / 10000) % 10;

    switch (len)
    {
    case 0:
      // sending commands to clear all 5 displays
      setflip(char2, 35);
      setflip(char1, 35);
      sendByte(i2cAddress1, 0xFF);
      sendByte(i2cAddress2, 0xFF);
      sendByte(i2cAddress3, 0xFF);
      // set the 3 ADTrans Modules to NONE
      break;
    case 1:
      // set char1 and clear char2 - char5
      setflip(char2, map(einer, 0, 9, 48, 57));
      setflip(char1, 35);
      sendByte(i2cAddress1, 0xFF);
      sendByte(i2cAddress2, 0xFF);
      sendByte(i2cAddress3, 0xFF);
      break;
    case 2:
      // set char1 & char2 and clear char3 - char5
      setflip(char2, map(zehner, 0, 9, 48, 57));
      setflip(char1, map(einer, 0, 9, 48, 57));
      sendByte(i2cAddress1, 0xFF);
      sendByte(i2cAddress2, 0xFF);
      sendByte(i2cAddress3, 0xFF);
      break;
    case 3:
      // set char1 & char2 & char3 and clear char4 - char5
      setflip(char2, map(hunderter, 0, 9, 48, 57));
      setflip(char1, map(zehner, 0, 9, 48, 57));
      sendByte(i2cAddress1, 0xFF);
      sendByte(i2cAddress2, 0xFF);
      sendByte(i2cAddress3, bin2grayi(einer));
      break;
    case 4:
      // set char1, char2, char3, char4 and clear char5
      setflip(char2, map(tausender, 0, 9, 48, 57));
      setflip(char1, map(hunderter, 0, 9, 48, 57));
      sendByte(i2cAddress3, bin2grayi(zehner));
      sendByte(i2cAddress2, bin2grayi(einer));
      sendByte(i2cAddress1, 0xFF);
      break;
    case 5:
      // set char1, char2, char3, char4, char5
      setflip(char2, map(ztausender, 0, 9, 48, 57));
      setflip(char1, map(tausender, 0, 9, 48, 57));
      sendByte(i2cAddress3, bin2grayi(hunderter));
      sendByte(i2cAddress2, bin2grayi(zehner));
      sendByte(i2cAddress1, bin2grayi(einer));
      break;
    default:
      break;
    }
  }
  
  
  // Abfahrtszeit
  if (abstd != 0 && abmin != 0)
  {
    setflip(stdadress, map(abstd, 0, 23, 34, 57));
    setflip(minadress, map(abmin, 0, 59, 34, 93));
  }

  setflip(logoadress, betreiber);
  setflip(gattungadress, gattung);
  setflip(infoadress, info1);
  setflip(infoadress2, info2);
  setflip(zieladress, ziel);
  setflip(zwischenzieladress, zwischenziel);

  updateModule2Direct();
}

// convert number to Gray code for the ADTrans modules, because they are not binary coded
byte bin2grayi(byte n)
{
  n += 1;
  return (~(n ^ (n >> 1)) & ((1 << 8) - 1));
}

// Sorgt fuer ein Leerblaettern der Module
// 0x82 klappt nicht bei allen Modulen, daher wurde einfach alles einzeln angesteuert
void clear()
{
  if (laststatus != 0)
  {
    // Serial.write(0x82);
    delay(100);
    setflip(stdadress, 32);
    setflip(minadress, 32);
    setflip(char2, 32);
    setflip(char1, 32);
    setflip(logoadress, 130);
    setflip(gattungadress, 32);
    setflip(infoadress, 32);
    setflip(infoadress2, 32);
    setflip(zieladress, 32);
    setflip(zwischenzieladress, 103);

    sendByte(i2cAddress1, 0xFF);
    sendByte(i2cAddress2, 0xFF);
    sendByte(i2cAddress3, 0xFF);

    laststatus = 0;
  }
}

// Gibt die Daten aus nach dem Krone-Protokoll
void setflip(byte adress, byte flip)
{

  for (int i = 0; i <= 0; i++)
  {
    Serial2.write(0x88);   // 88 Beginn Übertrag
    Serial2.write(adress); //  Adresse
    Serial2.write(flip);   //  Char
    Serial2.write(0x81);   // 81 Ende Übertrag
    delay(20);
  }

  laststatus = 1;
} // End setflip()

void module2SendCommand(uint8_t command, uint8_t address, const uint8_t *payload, size_t payloadLen)
{
  uint8_t frame[8];
  size_t frameLen = 0;

  frame[frameLen++] = 0xFF;
  frame[frameLen++] = command;
  frame[frameLen++] = address;

  for (size_t i = 0; i < payloadLen && frameLen < sizeof(frame); ++i)
  {
    frame[frameLen++] = payload[i];
  }

#if UART_MODULE2_RS485_DE_PIN >= 0
  digitalWrite(UART_MODULE2_RS485_DE_PIN, HIGH);
  delayMicroseconds(80);
#endif

  // Protokoll fordert BREAK vor dem Frame.
  const uint32_t breakUs = (static_cast<uint32_t>(UART_MODULE2_BREAK_BITS) * 1000000UL + UART_MODULE2_BAUD - 1) / UART_MODULE2_BAUD;
  Serial1.flush();
  Serial1.end();
  pinMode(UART_MODULE2_TX_PIN, OUTPUT);
  digitalWrite(UART_MODULE2_TX_PIN, LOW);
  delayMicroseconds(breakUs);
  digitalWrite(UART_MODULE2_TX_PIN, HIGH);
  delayMicroseconds(40);

  Serial1.begin(UART_MODULE2_BAUD, SERIAL_8N1, UART_MODULE2_RX_PIN, UART_MODULE2_TX_PIN);

  while (Serial1.available() > 0)
  {
    Serial1.read();
  }

  Serial1.write(frame, frameLen);
  Serial1.flush();

#if UART_MODULE2_RS485_DE_PIN >= 0
  digitalWrite(UART_MODULE2_RS485_DE_PIN, LOW);
#endif
}

void module2SetPosition(uint8_t address, uint8_t position)
{
  uint8_t payload[1] = {position};
  module2SendCommand(0xC0, address, payload, 1);
}

void updateModule2Direct()
{
  #if MODULE2_DIRECT_ENABLED
  module2SetPosition(static_cast<uint8_t>(MODULE2_DIRECT_ADDR), static_cast<uint8_t>(ziel & 0xFF));
  #endif
}


void sendByte(byte i2cAddress, byte zahl)
{
  Wire.beginTransmission(i2cAddress);
  Wire.write(zahl);
  Wire.endTransmission();
}

// Fuer WiFi-AP
void returnConfigPage()
{
  server.send(200, "text/html", MAIN_page);
}
