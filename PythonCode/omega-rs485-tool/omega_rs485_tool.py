#!/usr/bin/env python3
"""Omega RS485 command sender.

Sends frames in the format:
    <BREAK> FF <CMD> <ADDR> [DATA...]

Protocol reference in this workspace:
    doc/sbb-fallblatt-master/doc/protocol_new_modules.md
"""

from __future__ import annotations

import argparse
import time
from typing import Iterable, List

import serial


def parse_u8(value: str) -> int:
    """Parse an 8-bit value from decimal or hex (with or without 0x)."""
    raw = value.strip()
    try:
        parsed = int(raw, 0)
    except ValueError:
        parsed = int(raw, 16)

    if not 0 <= parsed <= 0xFF:
        raise argparse.ArgumentTypeError(f"Wert ausserhalb 0..255: {value}")
    return parsed


def parse_range(value: str) -> range:
    """Parse address range in the form START-END (inclusive)."""
    if "-" not in value:
        raise argparse.ArgumentTypeError("Bereich muss als START-END angegeben werden, z. B. 0-255")

    start_text, end_text = value.split("-", 1)
    start = parse_u8(start_text)
    end = parse_u8(end_text)

    if start > end:
        raise argparse.ArgumentTypeError("START muss <= END sein")

    return range(start, end + 1)


def format_hex(data: Iterable[int]) -> str:
    return " ".join(f"{b:02X}" for b in data)


def send_with_break(port: serial.Serial, frame: bytes, break_ms: float) -> None:
    # Hold line in BREAK before each frame as required by protocol.
    port.break_condition = True
    time.sleep(break_ms / 1000.0)
    port.break_condition = False
    time.sleep(0.001)

    port.write(frame)
    port.flush()


def send_via_esp32_bridge(port: serial.Serial, cmd: int, addr: int, data: List[int]) -> None:
    tokens = ["M2", f"{cmd:02X}", f"{addr:02X}"] + [f"{b:02X}" for b in data]
    line = " ".join(tokens) + "\n"
    port.write(line.encode("ascii"))
    port.flush()


def read_bridge_feedback(port: serial.Serial) -> List[str]:
    lines: List[str] = []

    while True:
        raw = port.readline()
        if not raw:
            break

        text = raw.decode("ascii", errors="replace").strip()
        if not text:
            continue

        lines.append(text)

        if text.startswith("M2 OK") or text.startswith("M2 ERR"):
            break

    return lines


def collect_addresses(args: argparse.Namespace) -> List[int]:
    if args.all:
        return list(range(0, 256))

    if args.range is not None:
        return list(args.range)

    return args.addr


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Omega RS485 Tool: send command bytes to addresses 0..255"
    )
    parser.add_argument("--port", required=True, help="Serieller Port, z. B. /dev/cu.usbserial-0001")
    parser.add_argument(
        "--baud",
        type=int,
        default=None,
        help="Baudrate (Default: 19200 im Direktmodus, 115200 mit --via-esp32-usb)",
    )
    parser.add_argument("--timeout", type=float, default=0.2, help="Lese-Timeout in Sekunden (Default: 0.2)")

    parser.add_argument("--cmd", required=True, type=parse_u8, help="Kommando-Byte, z. B. C0")
    parser.add_argument(
        "--data",
        nargs="*",
        default=[],
        type=parse_u8,
        help="Optionale Daten-Bytes, z. B. --data 14 oder --data 01 02",
    )

    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--all", action="store_true", help="An alle Adressen 0..255 senden")
    group.add_argument("--range", type=parse_range, help="Adressbereich START-END, z. B. 0-255")
    group.add_argument(
        "--addr",
        action="append",
        type=parse_u8,
        help="Einzeladresse (mehrfach nutzbar), z. B. --addr 29 --addr 30",
    )

    parser.add_argument("--break-ms", type=float, default=2.0, help="BREAK-Dauer in ms (Default: 2.0)")
    parser.add_argument(
        "--delay-ms",
        type=float,
        default=0.0,
        help="Pause zwischen zwei Adressen in ms (Default: 0)",
    )
    parser.add_argument(
        "--read-bytes",
        type=int,
        default=0,
        help="Wenn >0, nach jedem Senden genau N Bytes lesen",
    )
    parser.add_argument("--dry-run", action="store_true", help="Nur anzeigen, nichts senden")
    parser.add_argument(
        "--via-esp32-usb",
        action="store_true",
        help="Sende als Textkommando an die ESP32-USB-Bridge statt direktes RS485-Binary",
    )

    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    addresses = collect_addresses(args)
    if not addresses:
        parser.error("Keine Adresse angegeben")

    if args.read_bytes < 0:
        parser.error("--read-bytes muss >= 0 sein")

    if args.via_esp32_usb and args.read_bytes > 0:
        parser.error("--read-bytes ist mit --via-esp32-usb nicht anwendbar")

    baud = args.baud
    if baud is None:
        baud = 115200 if args.via_esp32_usb else 19200

    mode = "ESP32-USB-Bridge" if args.via_esp32_usb else "Direkt-RS485"
    print(f"Port={args.port} Baud={baud} CMD={args.cmd:02X} Targets={len(addresses)} Mode={mode}")

    if args.dry_run:
        for addr in addresses:
            frame = bytes([0xFF, args.cmd, addr, *args.data])
            print(f"TX {addr:03d}: {format_hex(frame)}")
        return 0

    with serial.Serial(args.port, baud, timeout=args.timeout) as port:
        for idx, addr in enumerate(addresses):
            frame = bytes([0xFF, args.cmd, addr, *args.data])
            if args.via_esp32_usb:
                send_via_esp32_bridge(port, args.cmd, addr, args.data)
                print(f"TX {addr:03d}: M2 {args.cmd:02X} {addr:02X} {format_hex(args.data)}".rstrip())
                feedback = read_bridge_feedback(port)
                for line in feedback:
                    print(f"ESP32 {addr:03d}: {line}")
                if not feedback:
                    print(f"ESP32 {addr:03d}: <keine Rueckmeldung>")
            else:
                send_with_break(port, frame, args.break_ms)
                print(f"TX {addr:03d}: {format_hex(frame)}")

                if args.read_bytes > 0:
                    response = port.read(args.read_bytes)
                    if response:
                        print(f"RX {addr:03d}: {format_hex(response)}")
                    else:
                        print(f"RX {addr:03d}: <timeout>")

            if args.delay_ms > 0 and idx < (len(addresses) - 1):
                time.sleep(args.delay_ms / 1000.0)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
