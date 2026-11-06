# Changelog

## 1.1.0 – 2026-11-06

- **Webseite** (`wordclock-web`): WLAN wählen, Zeitzone und Uhrzeit stellen (auch RTC),
  Helligkeit, Nachtmodus, Passwort, Neustart/Ausschalten. Python-Standardbibliothek,
  läuft auf dem Raspberry Pi 1.
- **Setup-Hotspot** (`wordclock-wifi`): ohne WLAN-Verbindung eigener Hotspot mit
  Captive Portal (hostapd + dnsmasq, für Bullseye/dhcpcd). Installation mit `make install-wifi`.
- `piclock` liest `/etc/wordclock/wordclock.conf` (Helligkeit, Nachtmodus) und übernimmt
  Änderungen ohne Neustart; im Hotspot-Modus blinken die Minutenpunkte.
- Neue Optionen `piclock -c` und `-p`.
- `make check` testet zusätzlich Einstellungsdatei, WLAN-Skript und Webseite.

## 1.0.0 – 2026-11-01

Erste Version auf GitHub.

- Wortuhr in C für 4 × SAA1064 am Raspberry Pi (Schweizerdeutsch, 10 × 11 Buchstaben, 4 Minutenpunkte)
- Zentrale Wort-Tabelle, auf der Hardware verifiziert
- Korrekte Stunde ab :25 («FÜF VOR HAUBI …»)
- Control-Byte wird bei jedem Update gesendet (Erholung nach Chip-Reset)
- Testprogramm `test_leds` (einzelne LEDs, Wörter, freie Ausgänge, Überschneidungsprüfung)
- `make install` / `make uninstall` mit systemd-Dienst
- `make check`: Regressionstest für alle 1440 Minuten
- `tools/check_layout.py`: prüft die Wort-Tabelle gegen das Buchstabenraster
