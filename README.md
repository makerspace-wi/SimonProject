# Simon Project

ESP32-basierte Steuerung fuer Fallblattanzeigen (MAN/Krone/Vossloh) mit:
- WiFi Access Point
- Weboberflaeche/HTTP-API
- Serieller Ausgabe im Krone-Protokoll
- I2C-Transfer von Byte-Werten an 3 Slave-Adressen (ADTrans)

## Projektziel

Dieses Projekt stellt mit einem ESP32 einen lokalen Access Point bereit. Ueber HTTP-Endpoints koennen Anzeigeinhalte gesetzt oder geloescht werden. Die Ausgabe an die Fallblattmodule erfolgt seriell im Format:

- Startbyte: `0x88`
- Adresse: `adress`
- Wert: `flip`
- Endbyte: `0x81`

## Verwendete Plattform

- MCU/Board: ESP32 (`wemos_d1_uno32`)
- Framework: Arduino
- Build-System: PlatformIO
- Webserver: `ESP32WebServer`
- mDNS: `ESPmDNS`
- I2C: `Wire`

Konfiguration: [platformio.ini](platformio.ini)

## Build und Upload

### Build

```bash
~/.platformio/penv/bin/platformio run
```

### Upload (Beispiel)

```bash
~/.platformio/penv/bin/platformio run -t upload
```

## Netzwerkverhalten

Beim Start erzeugt der ESP32 einen Access Point:
- SSID: `fallblatt.local`
- Passwort: `Fallblattanzeige`

Zusatz: mDNS wird mit Hostname `fallblatt` initialisiert.

## HTTP-Endpoints

Implementierung in [src/main.cpp](src/main.cpp).

### 1) Startseite

- `GET /`
- Liefert die HTML-Oberflaeche aus `MAIN_page` (Datei [src/web.h](src/web.h)).

### 2) Anzeige loeschen

- `GET /clear`
- Loescht/neutralisiert alle angesteuerten Fallblattpositionen.

### 3) Anzeige setzen

- `GET /flip`
- Erwartete Query-Parameter:
  - `b` = Betreiber
  - `g` = Gattung
  - `n` = Zugnummer
  - `s` = Abfahrtsstunde
  - `m` = Abfahrtsminute
  - `i1` = Info 1
  - `i2` = Info 2
  - `d` = Zwischenziel
  - `z` = Ziel

Beispiel:

```text
/flip?b=1&g=2&n=42&s=12&m=34&i1=5&i2=6&d=7&z=8
```

Hinweis:
- Der Endpoint `/flip` steuert zusaetzlich 3 I2C-Module (ADTrans) ueber `I2C_ADDR_1..3`.
- Je nach Laenge der Zugnummer werden Ziffern auf UART-Fallblattmodule und I2C-Module verteilt.

### 4) Zweites Modul (direkt im Programm gesteuert)

Die zweite serielle Schnittstelle (19200 Baud, BREAK-Protokoll) wird ohne eigenen HTTP-Endpoint direkt im Programmablauf gesteuert.

Aktuell erfolgt die Ansteuerung am Ende von `/flip` mit:

- Zieladresse aus `MODULE2_DIRECT_ADDR`
- Position aus dem bereits berechneten Feld `ziel`

Damit geht die Kommunikation fuer das zweite Modul nicht separat ueber den Webserver.

## I2C-Konfiguration

Standardwerte in [platformio.ini](platformio.ini):

- `I2C_ADDR_1=0x38`
- `I2C_ADDR_2=0x39`
- `I2C_ADDR_3=0x3A`

Optionale Pinbelegung (auskommentiert):
- `I2C_SDA_PIN`
- `I2C_SCL_PIN`

Wenn beide Pins gesetzt sind, wird `Wire.begin(SDA, SCL)` verwendet.
Andernfalls nutzt das Projekt `Wire.begin()` mit Board-Default.

## Genutzte IO-Pins

Aktueller Stand gemaess [platformio.ini](platformio.ini) und [src/main.cpp](src/main.cpp):

- Debug-UART (`Serial`): ueber USB-Seriell (Monitor 115200 Baud).
- Signal-UART (`Serial2`, 4800 Baud, 8E2):
  - TX: GPIO17 (`UART_SIGNAL_TX_PIN`)
  - RX: GPIO16 (`UART_SIGNAL_RX_PIN`)
- Modul-2-UART (`Serial1`, 19200 Baud, 8N1, BREAK-faehig):
  - TX: GPIO26 (`UART_MODULE2_TX_PIN`)
  - RX: GPIO27 (`UART_MODULE2_RX_PIN`)
  - Optional RS485 Driver-Enable: `UART_MODULE2_RS485_DE_PIN` (Standard `-1` = aus)
- I2C (`Wire`):
  - Aktuell keine festen Pins im Build-Flag gesetzt.
  - Es werden daher die Board-Defaults verwendet (`Wire.begin()`).
  - Optional konfigurierbar ueber `I2C_SDA_PIN` und `I2C_SCL_PIN` in [platformio.ini](platformio.ini).

Hinweis:
- Auf vielen ESP32-Boards liegt USB-Debug intern auf UART0 (typisch TX0=GPIO1, RX0=GPIO3).
- Fuer die Fallblattansteuerung wird ausschliesslich `Serial2` genutzt.
- Die ADTrans-I2C-Module werden mit `sendByte(...)` angesteuert (kein separater HTTP-I2C-Endpoint).

## Fallblatt-Adressen (seriell)

Die adressierten Module sind in [src/main.cpp](src/main.cpp) festgelegt, u. a.:
- Logo: `0x05`
- Gattung: `0x04`
- Zeichen 1/2: `0x03`, `0x02`
- Stunde/Minute: `0x01`, `0x00`
- Info 1/2: `0x07`, `0x06`
- Zwischenziel/Ziel: `0x08`, `0x09`

## Troubleshooting

### Build-Task in VS Code schlaegt fehl, Terminal-Build funktioniert

Symptom:
- VS-Code-Task meldet Fehler, manueller Build ist erfolgreich.

Pruefung/Fix:
- Sicherstellen, dass [/.vscode/tasks.json](.vscode/tasks.json) den PlatformIO-Pfad aus dem Python-Environment nutzt.
- Build direkt testen:

```bash
~/.platformio/penv/bin/platformio run
```

### Include-Fehler / IntelliSense findet Header nicht

Symptom:
- Meldung wie "Aktualisieren Sie Ihren includePath".

Pruefung/Fix:
- Einmal erfolgreichen Build ausfuehren, damit PlatformIO die Projektmetadaten aktualisiert.
- VS Code neu laden, falls die Meldung bestehen bleibt.
- In [platformio.ini](platformio.ini) das richtige Environment pruefen (`ESP32_WROOM`).

### Upload funktioniert nicht

Symptom:
- `run -t upload` bricht mit Port- oder Verbindungsfehler ab.

Pruefung/Fix:
- USB-Kabel/Port wechseln.
- Board-Typ in [platformio.ini](platformio.ini) pruefen (`wemos_d1_uno32`).
- Upload separat testen:

```bash
~/.platformio/penv/bin/platformio run -t upload
```

### I2C-Geraet reagiert nicht

Symptom:
- Ziffern auf den ADTrans-Modulen fehlen oder bleiben auf Leerwert.

Pruefung/Fix:
- I2C-Adressen in [platformio.ini](platformio.ini) mit den Slave-Adressen abgleichen (`I2C_ADDR_1..3`).
- Falls eigene Pins noetig sind, `I2C_SDA_PIN` und `I2C_SCL_PIN` in [platformio.ini](platformio.ini) aktivieren.
- GND zwischen ESP32 und I2C-Slaves gemeinsam verbinden.

### mDNS/Hostname nicht erreichbar

Symptom:
- `fallblatt.local` wird nicht aufgeloest.

Pruefung/Fix:
- Direkt per IP aufrufen statt Hostname.
- Sicherstellen, dass das Endgeraet im Access-Point des ESP32 eingebucht ist.
- AP-Daten pruefen: SSID `fallblatt.local`, Passwort `Fallblattanzeige`.

## Hinweise

- Das Projekt ist fuer die aktuelle ESP32-PlatformIO-Umgebung ausgelegt.
- Bei Problemen zuerst Build lokal pruefen und die Werte in [platformio.ini](platformio.ini) kontrollieren.

1. OMEGA-Modul über RS485 ansteuern
Das OMEGA-Modul hängt an Serial1 (RS485, Pins 26/27, 19200 Baud, 8N1). Du hast bereits die Funktion module2SetPosition(), die den Befehl 0xC0 (Set Position) sendet. Diese kannst du für das OMEGA-Modul verwenden – aber mit der richtigen Adresse deines OMEGA-Moduls.

Statt:

cpp
setflip(logoadress, betreiber);
schreibst du:

cpp
// Betreiber an OMEGA-Modul über RS485 senden
// MODULE2_DIRECT_ADDR muss die tatsächliche Adresse des OMEGA-Moduls sein
module2SetPosition(static_cast<uint8_t>(MODULE2_DIRECT_ADDR), static_cast<uint8_t>(betreiber & 0xFF));


Super einfach, wir machen es Schritt für Schritt.
Das Tool ist ein kleines Programm, das über die serielle RS485-Schnittstelle Befehle an Module mit Adresse 0 bis 255 sendet.

Siehe auch: README.md

Was das Tool macht

Es sendet immer ein Telegramm im Format: BREAK + FF + COMMAND + ADDR + optionale Daten.

Du gibst an:
Port, Command, und Zieladresse(n).

Zieladressen können sein:
eine Adresse, ein Bereich oder alle 0..255.

Einmalige Einrichtung

Terminal im Projekt öffnen und in den Ordner wechseln:
cd "/Users/dieter/Documents/PlatformIO/Projects/Simon Project/omega-rs485-tool"

Virtuelle Umgebung anlegen:
python3 -m venv .venv

Aktivieren:
source .venv/bin/activate

Abhängigkeit installieren:
pip install -r requirements.txt

Erster Test ohne Senden

Damit siehst du nur, was gesendet würde:
python omega_rs485_tool.py --port /dev/cu.usbserial-0001 --cmd C8 --addr 29 --dry-run

Wenn das gut aussieht: --dry-run entfernen.

Einfaches echtes Senden

TEST-Command C8 an Adresse 29:
python omega_rs485_tool.py --port /dev/cu.usbserial-0001 --cmd C8 --addr 29

An alle Adressen 0..255:
python omega_rs485_tool.py --port /dev/cu.usbserial-0001 --cmd C8 --all

Beispiel mit Antwort lesen

Firmware-Version lesen, erwartet 2 Antwort-Bytes:
python omega_rs485_tool.py --port /dev/cu.usbserial-0001 --cmd D4 --all --read-bytes 2 --delay-ms 5

Ausgabe zeigt dann pro Adresse TX und ggf. RX.

Häufige Stolpersteine

Falscher Port: dann kommt meist sofort ein Öffnungsfehler.

Falsche Baudrate: hier ist 19200 Standard.

RS485-Richtung/Adapter: bei manchen Adaptern muss DE/RE korrekt geschaltet sein.

Erst immer mit einer Adresse testen, dann Bereich oder all.

Wenn du willst, gehen wir jetzt zusammen live durch deinen ersten echten Lauf, und ich sage dir genau, welchen Befehl du als Nächstes eingibst.


Top, dann ist hier Kommando 2 (Full-Scan 0..255 mit TEST-Befehl C8):

~/.platformio/penv/bin/python omega_rs485_tool.py --port /dev/cu.usbserial-0001 --cmd C8 --all --delay-ms 5

Optional erst als Trockenlauf:

~/.platformio/penv/bin/python omega_rs485_tool.py --port /dev/cu.usbserial-0001 --cmd C8 --all --delay-ms 5 --dry-run

So gibst du den seriellen Port wieder frei:

Laufendes Python-Skript stoppen
Im Terminal, in dem das Tool läuft: Control + C
Erst dadurch wird die serielle Verbindung geschlossen.
Virtuelle Umgebung verlassen
Befehl: deactivate
Falls Port trotzdem noch blockiert ist
Prüfen, welcher Prozess den Port hält:
lsof | grep cu.usbserial
Dann den betreffenden Prozess beenden:
kill -9 PID
Kurz gesagt:

Für den Port ist vor allem Control + C wichtig.
deactivate beendet nur die venv, nicht automatisch ein laufendes Skript.
