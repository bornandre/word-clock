#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
LED-Test für eine Wortuhr mit SAA1064 LED-Treibern am Raspberry Pi 4 (I2C).

Jede LED wird einzeln eingeschaltet. Dabei wird ihre Nummer und Position
(Chip / Digit / Segment) ausgegeben, damit du notieren kannst, welcher
Buchstabe leuchtet. Daraus erstellen wir anschliessend das Mapping.

LED-Nummerierung (global, ab 0):
    Nr = Chip-Index * LEDS_PRO_CHIP + Digit * 8 + Segment

Aufruf:
    python3 led_test.py                 # automatischer Durchlauf (Endlosschleife)
    python3 led_test.py --delay 0.5     # schneller
    python3 led_test.py --manuell       # mit Enter weiterschalten (ideal fürs Mapping)
    python3 led_test.py --csv led_liste.csv   # Vorlage für das Mapping erzeugen
    python3 led_test.py --alle-an       # alle LEDs an (Test)
"""

import argparse
import csv
import sys
import time

try:
    from smbus2 import SMBus
except ImportError:
    print("Bitte zuerst installieren:  pip3 install smbus2")
    sys.exit(1)

# ---------------------------------------------------------------------------
# Konfiguration – bei Bedarf anpassen
# ---------------------------------------------------------------------------
I2C_BUS = 1  # Raspberry Pi 4: Bus 1 (GPIO2 = SDA, GPIO3 = SCL)

# 7-Bit-Adressen der SAA1064 (abhängig von der Spannung am ADR-Pin):
#   ADR = GND -> 0x38 | 3/8 VCC -> 0x39 | 5/8 VCC -> 0x3A | VCC -> 0x3B
# (entspricht 0x70 / 0x72 / 0x74 / 0x76 im 8-Bit-Format des Datenblatts)
CHIP_ADRESSEN = [0x38, 0x39, 0x3A, 0x3B]

# True  = Multiplex-Betrieb: 4 Digits x 8 Segmente = 32 LEDs pro Chip
# False = statischer Betrieb: 2 Digits x 8 Segmente = 16 LEDs pro Chip
MULTIPLEX = True

# Segmentstrom in mA: 0, 3, 6, 9, 12, 15, 18 oder 21
STROM_MA = 12

# Register (Subadressen) des SAA1064
REG_CONTROL = 0x00
REG_DIGIT1 = 0x01

DIGITS_PRO_CHIP = 4 if MULTIPLEX else 2
LEDS_PRO_CHIP = DIGITS_PRO_CHIP * 8


def control_byte(test_alle_segmente=False):
    """Steuerbyte gemäss Datenblatt aufbauen.

    Bit 0 (C0): 1 = dynamischer (Multiplex-)Betrieb
    Bit 1 (C1): 1 = Digits 1+3 aktiv
    Bit 2 (C2): 1 = Digits 2+4 aktiv
    Bit 3 (C3): 1 = Segmenttest (alle Segmente an)
    Bit 4..6  : Segmentstrom +3 / +6 / +12 mA
    """
    c = 0
    if MULTIPLEX:
        c |= 0x01
    c |= 0x02 | 0x04
    if test_alle_segmente:
        c |= 0x08
    stufen = max(0, min(7, STROM_MA // 3))
    c |= stufen << 4
    return c


class Saa1064Kette:
    """Verwaltet mehrere SAA1064 am selben I2C-Bus."""

    def __init__(self, bus_nr, adressen):
        self.bus = SMBus(bus_nr)
        self.adressen = []
        for adr in adressen:
            try:
                self.bus.write_i2c_block_data(adr, REG_CONTROL, [control_byte(), 0, 0, 0, 0])
                self.adressen.append(adr)
                print(f"  SAA1064 gefunden auf 0x{adr:02X}")
            except OSError:
                print(f"  WARNUNG: kein SAA1064 auf 0x{adr:02X} – wird übersprungen")
        if not self.adressen:
            print("Kein SAA1064 erreichbar. I2C aktiviert? Verdrahtung? (i2cdetect -y 1)")
            sys.exit(1)

    @property
    def anzahl_leds(self):
        return len(self.adressen) * LEDS_PRO_CHIP

    def schreibe_chip(self, adr, digits, test=False):
        daten = [control_byte(test)] + list(digits) + [0] * (4 - len(digits))
        self.bus.write_i2c_block_data(adr, REG_CONTROL, daten)

    def alle_aus(self):
        for adr in self.adressen:
            self.schreibe_chip(adr, [0] * DIGITS_PRO_CHIP)

    def alle_an(self):
        for adr in self.adressen:
            self.schreibe_chip(adr, [0xFF] * DIGITS_PRO_CHIP)

    @staticmethod
    def position(nr):
        """Globale LED-Nummer -> (Chip-Index, Digit, Segment)."""
        chip = nr // LEDS_PRO_CHIP
        rest = nr % LEDS_PRO_CHIP
        return chip, rest // 8, rest % 8

    def setze_einzel_led(self, nr):
        """Schaltet genau eine LED ein, alle anderen aus."""
        chip_idx, digit, segment = self.position(nr)
        for i, adr in enumerate(self.adressen):
            digits = [0] * DIGITS_PRO_CHIP
            if i == chip_idx:
                digits[digit] = 1 << segment
            self.schreibe_chip(adr, digits)

    def beschreibung(self, nr):
        chip_idx, digit, segment = self.position(nr)
        return (f"LED {nr:3d}  |  Chip 0x{self.adressen[chip_idx]:02X}  "
                f"Digit {digit + 1}  Segment {segment}")

    def schliessen(self):
        try:
            self.alle_aus()
        finally:
            self.bus.close()


def csv_vorlage(kette, dateiname):
    with open(dateiname, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f, delimiter=";")
        w.writerow(["nr", "chip", "digit", "segment", "buchstabe"])
        for nr in range(kette.anzahl_leds):
            chip_idx, digit, segment = kette.position(nr)
            w.writerow([nr, f"0x{kette.adressen[chip_idx]:02X}", digit + 1, segment, ""])
    print(f"Vorlage gespeichert: {dateiname}")


def lauf_automatisch(kette, delay):
    print(f"\nAutomatischer Durchlauf ({kette.anzahl_leds} LEDs, {delay}s). Abbruch mit Strg+C.\n")
    runde = 1
    while True:
        print(f"--- Runde {runde} ---")
        for nr in range(kette.anzahl_leds):
            kette.setze_einzel_led(nr)
            print(kette.beschreibung(nr))
            time.sleep(delay)
        runde += 1


def lauf_manuell(kette):
    print("\nManueller Modus:")
    print("  Enter = nächste LED | z = zurück | Zahl = zu LED springen | q = Ende\n")
    nr = 0
    while True:
        kette.setze_einzel_led(nr)
        eingabe = input(kette.beschreibung(nr) + "  > ").strip().lower()
        if eingabe == "q":
            break
        elif eingabe == "z":
            nr = (nr - 1) % kette.anzahl_leds
        elif eingabe.isdigit():
            nr = int(eingabe) % kette.anzahl_leds
        else:
            nr = (nr + 1) % kette.anzahl_leds


def main():
    p = argparse.ArgumentParser(description="SAA1064 LED-Test für Wortuhr")
    p.add_argument("--delay", type=float, default=1.0, help="Sekunden pro LED (Standard 1.0)")
    p.add_argument("--manuell", action="store_true", help="mit Enter weiterschalten")
    p.add_argument("--alle-an", action="store_true", help="alle LEDs einschalten und warten")
    p.add_argument("--csv", metavar="DATEI", help="CSV-Vorlage für das Mapping erzeugen")
    args = p.parse_args()

    print("Suche SAA1064 ...")
    kette = Saa1064Kette(I2C_BUS, CHIP_ADRESSEN)
    print(f"{len(kette.adressen)} Chip(s), {kette.anzahl_leds} LEDs, "
          f"{'Multiplex' if MULTIPLEX else 'statisch'}, {STROM_MA} mA")

    try:
        if args.csv:
            csv_vorlage(kette, args.csv)
        elif args.alle_an:
            kette.alle_an()
            input("Alle LEDs an. Enter zum Ausschalten ...")
        elif args.manuell:
            lauf_manuell(kette)
        else:
            lauf_automatisch(kette, args.delay)
    except KeyboardInterrupt:
        print("\nAbgebrochen.")
    finally:
        kette.schliessen()
        print("Alle LEDs aus.")


if __name__ == "__main__":
    main()
