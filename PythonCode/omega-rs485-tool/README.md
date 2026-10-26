# Omega RS485 Tool

Kleines CLI-Tool zum Senden von Omega-Modul-Kommandos ueber RS485.

Protokoll (laut Doku im Repository):

- Baudrate: 19200 8N1
- Frame: `<BREAK> FF <CMD> <ADDR> [DATA...]`

Quelle: `doc/sbb-fallblatt-master/doc/protocol_new_modules.md`

## Setup

```bash
cd omega-rs485-tool
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

Wenn `pip` wegen DNS/Internet nicht funktioniert, kannst du direkt den Python-Interpreter von PlatformIO nutzen (dort ist `pyserial` in der Regel bereits vorhanden):

```bash
~/.platformio/penv/bin/python omega_rs485_tool.py --port /dev/cu.usbserial-0001 --cmd C8 --addr 29 --dry-run
```

## Nutzung

```bash
python omega_rs485_tool.py --port COM9 --cmd C8 --all --data 51 --delay-ms 1000 --via-esp32-usb
```

Wichtige Optionen:

- `--addr N` einzelne Adresse (mehrfach moeglich)
- `--range A-B` Bereich inklusiv, z. B. `0-255`
- `--all` alle Adressen `0..255`
- `--cmd` Kommando-Byte, z. B. `C0`, `C8`, `D4`
- `--data` optionale Datenbytes
- `--break-ms` BREAK-Dauer in ms (Default `2.0`)
- `--read-bytes N` nach Senden N Bytes lesen
- `--delay-ms` Pause zwischen Adressen
- `--dry-run` nur anzeigen, nicht senden
- `--via-esp32-usb` sendet Textkommandos an die ESP32-USB-Bridge

Modi:

- Direkt-RS485 (Standard): Python spricht einen USB-RS485-Adapter direkt an (`BREAK + FF CMD ADDR ...`).
- ESP32-USB-Bridge (`--via-esp32-usb`): Python sendet ASCII-Kommandos (`M2 C0 0D 14`) an den ESP32-USB-Port; der ESP32 sendet dann ueber GPIO17/GPIO16 auf RS485.

## Beispiele

1) TEST (`C8`) an Adresse 29:

```bash
python omega_rs485_tool.py --port /dev/cu.usbserial-0001 --cmd C8 --addr 29
```

2) DISP (`C0`) Position 20 an Adresse 29:

```bash
python omega_rs485_tool.py --port /dev/cu.usbserial-0001 --cmd C0 --addr 29 --data 20
```

3) Firmware-Version lesen (`D4`) fuer alle Adressen 0..255, 2 Bytes Antwort:

```bash
python omega_rs485_tool.py --port /dev/cu.usbserial-0001 --cmd D4 --all --read-bytes 2 --delay-ms 5
```

4) Ueber ESP32-USB-Bridge senden (ESP32-Monitor-Baudrate 115200):

```bash
python omega_rs485_tool.py --port /dev/cu.usbserial-0001 --cmd C0 --addr 0D --data 14 --via-esp32-usb
```

## Hinweis

Aendere Adressen mit `CE` vorsichtig, da sich Module sonst nicht mehr wie erwartet ansprechen lassen.
