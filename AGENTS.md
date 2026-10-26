# AGENTS.md

ESP32-Firmware und Tooling zur Ansteuerung von SBB/MAN/Krone-Fallblattanzeigen. Dokumentation und Codekommentare sind auf Deutsch (ASCII-transliteriert: `ue`/`oe`) — diesen Stil in neuen Kommentaren/Doks beibehalten.

## Aufbau

- `ProjectCode/` — die eigentliche Firmware (PlatformIO, ESP32, Arduino). `src/main.cpp` enthaelt die komplette Logik; `src/web.h` enthaelt die gesamte Web-Oberflaeche als `PROGMEM`-Raw-String. Hat ein eigenes `.git`.
- `SimpleCode/` — separate PlatformIO-Spielwiese mit eigenstaendigen PCF8574-/RS485-Testsketches (Multi-Board-Envs). Kein Teil des Firmware-Builds.
- `PythonCode/omega-rs485-tool/` — CLI zum direkten Senden von Omega-RS485-Frames vom Host (siehe dortiges README; Hardware noetig, ausser mit `--dry-run`).
- `ProjectCode/doc/sbb-fallblatt-master/` — massgebliche Protokoll-/Elektrik-Doku (alte vs. neue Module). `protocol_new_modules.md` ist die Quelle fuer das RS485-Frame-Format.
- `SimonCode/` ist leer; die zip/xls/png-Dateien im Root sind originale Referenzartefakte, keine Build-Ziele.

Es gibt keinen Build auf Top-Level — jedes PlatformIO-Projekt baut fuer sich.

## Befehle (innerhalb von `ProjectCode/` ausfuehren)

```bash
~/.platformio/penv/bin/platformio run            # bauen
~/.platformio/penv/bin/platformio run -t upload  # flashen
~/.platformio/penv/bin/platformio run -t monitor # 115200 Baud
```

Falls PlatformIOs Python nicht erreichbar ist, funktioniert `~/.platformio/penv/bin/python` auch fuer das RS485-Tool (pyserial ist dort vorinstalliert).

## Firmware-Stolperfallen

- **Die gesamte Hardware-Konfiguration liegt in den `-D`-Build-Flags in `ProjectCode/platformio.ini`** (UART-Pins/Bauds, BREAK-Bits, RS485-DE-Pin, `I2C_ADDR_1..3`, `MODULE2_DIRECT_ADDR`). `main.cpp` hat nur Fallback-`#ifndef`-Defaults — Verhalten nicht allein dort aendern. Aktives Env ist `nodemcu-32s` (README-/Kommentar-Erwaehnungen von `ESP32_WROOM`/`wemos_d1_uno32` sind veraltet).
- Die Pin-Zuordnung im README widerspricht `platformio.ini` (Signal-UART und Modul-2-UART vertauscht). **Es gilt `platformio.ini` / `main.cpp`:** Signal-UART (`Serial2`, 4800 8E2) TX=26/RX=27; Modul-2-UART (`Serial1`, RS485, 19200 8N1) TX=17/RX=16.
- Drei unabhaengige Ausgabepfade: klassischer Krone-Serielframe `[0x88][Adresse][Blatt][0x81]` ueber `Serial2`; Omega-RS485 `BREAK + FF + CMD + ADDR [DATA]` ueber `Serial1`; und I2C-Gray-Code-Bytes an 3 PCF8574-Slaves (ADTrans) via `sendByte()`.
- Die Web-Oberflaeche existiert doppelt: `src/web.h` (ausgeliefert) und `web_preview.html` (Browservorschau). Die HTML-Bodies sind identisch — **immer beide im Sync aendern**.
- USB-Bridge: Der Module-2-/RS485-Bus laesst sich ohne Neuflassen steuern, indem ASCII-Befehle wie `C0 0D 14` (oder `M2 C0 0D 14`) in den USB-Seriell-Monitor getippt werden. `HELP` listet die Optionen. Modul-Antworten werden nach jedem Senden als `M2 RX: ...` ausgegeben (Fenster via `-DUART_MODULE2_RX_WINDOW_MS`, Default 100 ms).
- Sicherheitshinweis des Original-Autors zum Flashen: ESP32 vor dem Upload unbedingt vom Anzeige-Adapter abstecken (geteilte Seriell-Leitungen).

## Serielle Port-Hygiene

USB-Seriell-Ports werden zwischen Firmware und Python-Tool geteilt. Laeuft `omega_rs485_tool.py`, zuerst mit Strg+C beenden (dadurch wird der Port geschlossen; `deactivate` tut das nicht), bevor geflasht oder der Monitor gestartet wird. Bleibt der Port trotzdem belegt: `lsof | grep cu.usbserial`.

## RS485-Tool-Hinweise

- Default 19200 8N1; erst eine einzelne Adresse testen, dann Bereiche/`--all`.
- Adressaenderungs-Befehl `CE` kann Module dauerhaft falsch adressieren — mit Vorsicht verwenden.

## Omega-Modul (2026-10-20 am realen Modul verifiziert)

- Das vorhandene RS485-Modul: Adresse `0x0D`, Firmware 5.0, Typ 01 (40 Blaetter). Lesebefehle (`D0`/`D4`/`DD`/`DE`) beantwortet es nur unter seiner echten Adresse (`==`-Vergleich). Schreibbefehle (`C5`/`C6`/`C0`) fuehrt es fuer alle Adressen **<= eigene Adresse** aus, darueber hinaus nicht (verifiziert: 0x05/0x0C/0x0D akzeptiert, 0x0E/0x0F/0x10/0x32/0xFF abgelehnt). Vermutlich `<=` statt `==` in der Modul-Firmware. Bei mehreren Modulen kritisch: ein Schreibbefehl an Adresse k wuerde vermutlich alle Module mit Adresse >= k bewegen; `CE`-Adressaenderung nur mit einzelnem Modul am Bus ausfuehren.
- `C0` (DISP) antwortet protokollgemaess nicht; Erfolg ueber `D0`-Ruecklesen pruefen. Zeichenzuordnung: `doc/sbb-fallblatt-master/doc/char_mapping.md`. Abweichung am realen Modul: Position 0 (`C5`-Nullpunkt) zeigt das Blatt "ICE", nicht "0." wie in der Doku — Blattrung weicht vom Standard ab.
