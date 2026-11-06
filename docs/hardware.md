# Hardware

## Bauteile

| Anzahl | Teil | Bemerkung |
|-------:|------|-----------|
| 1 | Raspberry Pi (ab Modell 1 B) | für den Setup-Hotspot WLAN mit AP-Modus |
| 4 | SAA1064 | LED-Treiber mit I2C, 4 × 8 Ausgänge im Multiplex |
| 8 | Digit-Transistoren | 2 pro SAA1064 an MX1 / MX2, Typ gemäss Datenblatt-Schaltung |
| 4 | Kondensator (typ. 2,7 nF) | an Cext, bestimmt den Multiplex-Takt |
| 1 | I2C-Pegelwandler 3,3 V ↔ 5 V | z.B. mit BSS138, siehe unten |
| 114 | LED 5 mm | 110 Buchstaben + 4 Minutenpunkte |
| 1 | Netzteil 5 V, ≥ 3 A | für Pi und LEDs |

Transistoren und Kondensator gemäss der Multiplex-Beispielschaltung im Datenblatt
des SAA1064 wählen und dort nachprüfen, bevor du Platinen bestellst.

## I2C-Adressen

Die Adresse eines SAA1064 hängt von der Spannung am Pin **ADR** ab:

| ADR | 7-Bit-Adresse | im Datenblatt (8 Bit) |
|-----|:-------------:|:---------------------:|
| GND | `0x38` | `0x70` |
| 3/8 VCC | `0x39` | `0x72` |
| 5/8 VCC | `0x3A` | `0x74` |
| VCC | `0x3B` | `0x76` |

Die Zwischenwerte stellst du mit einem Spannungsteiler ein. Kontrolle auf dem Pi:

```bash
i2cdetect -y 1        # sollte 38 39 3a 3b anzeigen
```

## Anschluss am Raspberry Pi

| Pi-Pin | Signal | geht an |
|-------:|--------|---------|
| 1 | 3,3 V | Pegelwandler LV |
| 3 | GPIO2 / SDA | Pegelwandler LV1 → SDA aller SAA1064 |
| 5 | GPIO3 / SCL | Pegelwandler LV2 → SCL aller SAA1064 |
| 6 | GND | gemeinsame Masse |
| 2 / 4 | 5 V | Pegelwandler HV, VCC der SAA1064 |

### Warum ein Pegelwandler?

Der SAA1064 läuft mit 5 V und erkennt ein High-Signal erst ab etwa 0,7 × VCC = 3,5 V.
Der Pi liefert nur 3,3 V. Ohne Pegelwandler funktioniert es manchmal, aber unzuverlässig.
Die Pull-up-Widerstände auf der 5-V-Seite dürfen **nicht** direkt an die Pi-Pins.

## Helligkeit

Der SAA1064 regelt den Strom für alle Segmente eines Chips gemeinsam (3–21 mA).
Standard im Code: 3 mA. Anders einstellen beim Kompilieren:

```bash
make CFLAGS_EXTRA='-DSAA1064_CTRL_DEFAULT=0x47'   # 12 mA
make CFLAGS_EXTRA='-DSAA1064_CTRL_DEFAULT=0x77'   # 21 mA (Maximum)
```

Aufbau des Control-Bytes: Bit 0 = Multiplex, Bit 1/2 = Digits ein, Bit 4/5/6 = +3/+6/+12 mA.

## Gleichmässige Ausleuchtung der Buchstaben

5-mm-LEDs strahlen eng und erzeugen einen hellen Fleck in der Buchstabenmitte. Was hilft:

1. **Lichtkammern:** ein Raster (MDF, Karton oder 3D-Druck), das jede LED abschirmt,
   15–25 mm tief, innen **weiss matt** gestrichen. Verhindert zusätzlich das Übersprechen
   auf Nachbarbuchstaben.
2. **LEDs mattieren:** die Kuppe mit Schleifpapier (Körnung 400–600) anschleifen,
   oder gleich diffuse LEDs bzw. Typen mit 60–120° Abstrahlwinkel verwenden.
3. **Diffusor** hinter der Frontplatte: opales Acrylglas 2–3 mm,
   günstiger mit Transparent- oder Backpapier.
4. Den Lichtverlust mit höherem Segmentstrom ausgleichen (siehe oben).
