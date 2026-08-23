# Simon Project

ESP32-basierte Steuerung fuer Fallblattanzeigen (MAN/Krone/Vossloh) mit:
- WiFi Access Point
- Weboberflaeche/HTTP-API
- Serieller Ausgabe im Krone-Protokoll
- I2C-Transfer von Integer-Werten an 4 Slave-Adressen

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

### 4) I2C Integer an 4 Adressen senden

- `GET /i2c`
- Erwartete Query-Parameter:
  - `v1`, `v2`, `v3`, `v4`
- Zuordnung:
  - `v1` -> `I2C_ADDR_1`
  - `v2` -> `I2C_ADDR_2`
  - `v3` -> `I2C_ADDR_3`
  - `v4` -> `I2C_ADDR_4`

Beispiel:

```text
/i2c?v1=100&v2=200&v3=300&v4=400
```

Antwort:

```text
i2c_rc=<rc1>,<rc2>,<rc3>,<rc4>
```

`rc` ist der Rueckgabecode von `Wire.endTransmission()` pro Zieladresse.

## I2C-Konfiguration

Standardwerte in [platformio.ini](platformio.ini):

- `I2C_ADDR_1=0x10`
- `I2C_ADDR_2=0x11`
- `I2C_ADDR_3=0x12`
- `I2C_ADDR_4=0x13`

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
- I2C (`Wire`):
  - Aktuell keine festen Pins im Build-Flag gesetzt.
  - Es werden daher die Board-Defaults verwendet (`Wire.begin()`).
  - Optional konfigurierbar ueber `I2C_SDA_PIN` und `I2C_SCL_PIN` in [platformio.ini](platformio.ini).

Hinweis:
- Auf vielen ESP32-Boards liegt USB-Debug intern auf UART0 (typisch TX0=GPIO1, RX0=GPIO3).
- Fuer die Fallblattansteuerung wird ausschliesslich `Serial2` genutzt.

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
- Rueckgabe `i2c_rc` ist ungleich `0`.

Pruefung/Fix:
- I2C-Adressen in [platformio.ini](platformio.ini) mit den Slave-Adressen abgleichen (`I2C_ADDR_1..4`).
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
