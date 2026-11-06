# word-clock

Schweizerdeutsche Wortuhr für den Raspberry Pi mit vier **SAA1064**-LED-Treibern am I2C-Bus.

*A Swiss German word clock for the Raspberry Pi, driving 114 LEDs through four SAA1064
I2C LED drivers. Written in plain C, no dependencies, runs as a systemd service.*

```
E S K I S C H A F Ü F        ES ISCH FÜF VOR HAUBI DRÜ      (14:25)
V I E R T U B F Z Ä Ä        ES ISCH VIERTU AB ZÄNI         (10:15)
Z W Ä N Z G S I V O R        ES ISCH ZWÖUFI  • •            (12:02)
A B O H A U B I E P M
E I S Z W Ö I S D R Ü
V I E R I F Ü F I Q T
S Ä C H S I S I B N I
A C H T I N Ü N I E L
Z Ä N I E R B E L F I
Z W Ö U F I A M U H R
```

## Funktionen

- Anzeige in 5-Minuten-Schritten, dazu 4 Minutenpunkte für die Minuten dazwischen
- Reines C ohne externe Bibliotheken (nur Linux-I2C über `/dev/i2c-1`)
- Läuft als systemd-Dienst, schaltet die LEDs beim Beenden sauber aus
- Erholt sich selbst, wenn ein Chip durch einen Spannungseinbruch zurückgesetzt wird
- **Webseite** zum Einstellen von WLAN, Zeitzone, Uhrzeit, Helligkeit und Nachtmodus
- **Setup-Hotspot**: Ohne WLAN-Verbindung öffnet die Uhr ein eigenes WLAN mit Konfigurationsseite
- Nachtmodus mit eigener Helligkeit, Änderungen ohne Neustart
- Testprogramme zum Prüfen jeder einzelnen LED und jedes Worts
- Trockenlauf ohne Hardware und automatische Tests für alle 1440 Minuten und die Webseite

## Schnellstart

```bash
# einmalig: I2C aktivieren (Interface Options → I2C) und Werkzeuge installieren
sudo raspi-config
sudo apt install -y build-essential i2c-tools git
sudo usermod -aG i2c $USER        # danach neu anmelden

git clone https://github.com/bornandre/word-clock.git
cd word-clock/c
make
./piclock -t 14:25                # Testanzeige
sudo make install                 # Uhr + Webseite installieren und starten

# optional: Setup-Hotspot, falls kein WLAN erreichbar ist
sudo apt install -y hostapd dnsmasq-base iw
sudo make install-wifi
```

Danach ist die Uhr unter **http://piclock.local** erreichbar (Hostname des Pi + `.local`),
Passwort `wordclock`. Details: [Webseite und Setup-Hotspot](docs/webconfig.md).

Die Uhr zeigt die Systemzeit an. Prüf die Zeitzone:

```bash
timedatectl                                   # Zeitzone kontrollieren
sudo timedatectl set-timezone Europe/Zurich   # falls nötig
```

## Bedienung

### `piclock` – die Uhr

| Aufruf | Wirkung |
|--------|---------|
| `piclock` | normale Uhr |
| `piclock -v` | zusätzlich jede Minute den Satz ausgeben |
| `piclock -t 14:25` | feste Uhrzeit anzeigen (bis Strg+C) |
| `piclock -d` | Demo: alle 5-Minuten-Schritte eines halben Tages |
| `piclock -n -d` | Trockenlauf ohne Hardware, nur Textausgabe |
| `piclock -n -a` | alle 1440 Minuten als Text (für Tests) |
| `piclock -p` | gelesene Einstellungen ausgeben |
| `piclock -c DATEI` | andere Einstellungsdatei (Standard `/etc/wordclock/wordclock.conf`) |

### `test_leds` – Hardware prüfen

Nach `make install` heisst das Programm `piclock-test-leds`. Den Dienst vorher stoppen:
`sudo systemctl stop piclock`.

| Aufruf | Wirkung |
|--------|---------|
| `test_leds` | alle 128 Ausgänge einzeln nacheinander |
| `test_leds -m` | manuell: Enter = weiter, `z` = zurück, Zahl = springen, `q` = Ende |
| `test_leds -w -m` | jedes definierte Wort einzeln anzeigen |
| `test_leds -f -m` | nur Ausgänge, die keinem Wort zugeordnet sind |
| `test_leds -a` | alle LEDs an |
| `test_leds -c` | Wort-Tabelle auf Überschneidungen prüfen (ohne Hardware) |
| `test_leds -l` | freie Ausgänge auflisten (ohne Hardware) |

Ausgabe z.B. `LED 77 -> a3a[1] = 0x20`: Chip `0x3A`, Digit-Index 1, Segment-Bit `0x20`.

Ein entsprechendes Python-Testprogramm liegt in `tools/led_test.py` (benötigt `smbus2`).

### Dienst

```bash
systemctl status piclock wordclock-web wordclock-wifi
journalctl -u piclock -f          # Meldungen ansehen
sudo make uninstall               # Uhr + Webseite entfernen
sudo make uninstall-wifi          # Setup-Hotspot entfernen
```

`sudo make install` kopiert:

| Datei | Ziel |
|-------|------|
| `piclock` (bzw. `piclock32` auf 32 Bit) | `/usr/local/bin/piclock` |
| `test_leds` | `/usr/local/bin/piclock-test-leds` |
| `web/wordclock-web` | `/usr/local/sbin/wordclock-web` |
| `config/wordclock.conf`, `config/secret.conf` | `/etc/wordclock/` (nur wenn noch nicht vorhanden) |
| `systemd/piclock.service`, `systemd/wordclock-web.service` | `/etc/systemd/system/` |

`sudo make install-wifi` kopiert zusätzlich `wifi/wordclock-wifi` nach `/usr/local/sbin/`,
installiert `wordclock-wifi.service` und trägt `nohook wpa_supplicant` in `/etc/dhcpcd.conf` ein.

## Anpassen

- **Andere Verdrahtung / anderes Layout:** Wort-Tabelle in [`c/words.c`](c/words.c).
  Jedes Wort besteht aus bis zu 3 Teilen `P(Chip, Digit, Bitmaske)`.
- **Minuten-Logik** (z.B. «ZÄÄ VOR HAUBI» statt «ZWÄNZG AB»): Tabelle `minute_words` in `c/words.c`.
- **Helligkeit und Nachtmodus:** über die Webseite oder in `/etc/wordclock/wordclock.conf`.

Nach Änderungen prüfen:

```bash
make check                        # Überschneidungen, alle Minuten, Einstellungen, Webseite
python3 tools/check_layout.py     # stimmen die Wörter mit dem Buchstabenraster überein?
```

Wenn du die Ausgabe absichtlich änderst, erzeugst du die Referenz neu:
`./piclock -n -a > ../tests/expected_times.txt`

## Dokumentation

- [Hardware](docs/hardware.md): Bauteile, Anschluss, I2C-Adressen, Pegelwandler, Helligkeit, Ausleuchtung
- [Frontplatte und LED-Zuordnung](docs/layout.md): Raster, Verdrahtung jedes Buchstabens
- [Webseite und Setup-Hotspot](docs/webconfig.md): Einrichtung, Bedienung, Fehlersuche

## Projektstruktur

```
word-clock/
├── c/
│   ├── main.c          Uhr: Zeit lesen, Anzeige aktualisieren
│   ├── config.c/.h     Einstellungen, Helligkeit, Nachtmodus
│   ├── words.c/.h      Wort-Tabelle und Uhrzeit → LED-Muster
│   ├── saa1064.c/.h    Treiber für den SAA1064
│   ├── i2c.c/.h        Linux-I2C-Zugriff
│   ├── test_leds.c     Testprogramm für LEDs und Wörter
│   └── Makefile        make · make check · make install · make install-wifi
├── web/wordclock-web   Konfigurationsseite (Python, ohne Zusatzpakete)
├── wifi/wordclock-wifi WLAN mit Setup-Hotspot als Rückfallebene
├── config/             Vorlagen für /etc/wordclock
├── systemd/            piclock, wordclock-web, wordclock-wifi
├── tests/              Referenzausgaben, Einstellungs- und Webseiten-Tests
├── tools/
│   ├── led_test.py     LED-Test in Python
│   └── check_layout.py prüft words.c gegen das Buchstabenraster
└── docs/
```

## Lizenz

[MIT](LICENSE)
