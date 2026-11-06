# Webseite und Setup-Hotspot

Die Uhr hat eine eigene Konfigurationsseite. Ist kein bekanntes WLAN erreichbar,
öffnet sie ein eigenes WLAN («Setup-Hotspot»), über das man sie mit dem Handy einrichtet.

```
 Start
   │
   ▼
 bekanntes WLAN erreichbar? ──ja──▶ verbunden, Webseite unter http://piclock.local
   │ nein (nach 60 s)                     │
   ▼                                      │ Verbindung 2 min weg
 Setup-Hotspot «WordClock-Setup»  ◀───────┘
 Webseite unter http://10.42.0.1
 Minutenpunkte blinken
   │
   ├── WLAN auf der Webseite eingetragen ──▶ verbindet sich mit dem neuen WLAN
   └── 10 min kein Gerät verbunden ──────▶ versucht das bekannte WLAN erneut
```

## Voraussetzungen

- Raspberry Pi OS **Bullseye** mit **dhcpcd** (Standard bis Bullseye). Prüfen mit
  `grep PRETTY /etc/os-release` und `systemctl is-active dhcpcd`.
- WLAN-Stick oder eingebautes WLAN mit **Access-Point-Modus**:
  `iw list | grep -A 8 "Supported interface modes"` muss `* AP` enthalten.
- Python 3 (ist auf Raspberry Pi OS vorhanden).

Getestet ist der Aufbau für einen Raspberry Pi 1 Model B Rev 2 mit USB-WLAN-Stick und RTC.

## Installation

```bash
cd ~/git/word-clock/c
git pull
make
sudo make install                              # Uhr und Webseite

sudo apt install -y hostapd dnsmasq-base iw
sudo make install-wifi                         # Setup-Hotspot
```

> **Tipp:** Für `make install-wifi` den Pi am besten per **Netzwerkkabel** verbinden.
> Die WLAN-Verbindung bleibt dabei zwar normalerweise bestehen, aber falls etwas schiefgeht,
> kommst du über das Kabel weiterhin per SSH auf den Pi.

`make install-wifi`
- installiert `/usr/local/sbin/wordclock-wifi` und den Dienst `wordclock-wifi`,
- trägt `nohook wpa_supplicant` am Anfang von `/etc/dhcpcd.conf` ein: wpa_supplicant wird
  dann von `wordclock-wifi` gestartet statt von dhcpcd,
- deaktiviert den System-Dienst `hostapd` (der Hotspot wird nur bei Bedarf gestartet).

Rückgängig: `sudo make uninstall-wifi` und danach `sudo reboot`.

## Erste Einrichtung mit dem Handy

1. Uhr einschalten und gut eine Minute warten. Findet sie kein bekanntes WLAN,
   **blinken die vier Minutenpunkte**.
2. Mit dem Handy das WLAN **«WordClock-Setup»** wählen, Passwort **`wordclock`**.
3. Die Anmeldeseite öffnet sich meist von selbst. Sonst im Browser
   **http://10.42.0.1** öffnen.
4. Mit `wordclock` anmelden, unter «WLAN» das Heimnetz wählen und das Passwort eingeben.
5. Der Hotspot verschwindet, die Uhr verbindet sich. Das Handy wieder mit dem Heimnetz
   verbinden und die Uhr unter **http://piclock.local** aufrufen.

Auf manchen Android-Geräten muss man bestätigen, dass man «ohne Internet verbunden bleiben»
möchte.

## Die Seite

| Bereich | Funktion |
|---------|----------|
| Status | WLAN, Adresse, Zeitsynchronisation, RTC |
| WLAN | gespeicherte Netze, Netz suchen und verbinden, Netz entfernen |
| Zeit | Zeitzone; Datum und Uhrzeit von Hand oder «vom Handy übernehmen» (schreibt auch in die RTC) |
| Anzeige | Helligkeit, Nachtmodus mit Zeitfenster und eigener Helligkeit |
| Passwort | Passwort für Webseite und Hotspot ändern |
| System | Neu starten, Ausschalten, Abmelden |

Helligkeit und Nachtmodus übernimmt `piclock` innerhalb einer Sekunde, ohne Neustart.
Nach einer Änderung der Zeitzone wird `piclock` automatisch neu gestartet.

## Dateien

| Datei | Inhalt | Rechte |
|-------|--------|--------|
| `/etc/wordclock/wordclock.conf` | Helligkeit, Nachtmodus, Hotspot-Einstellungen | 644 |
| `/etc/wordclock/secret.conf` | Passwort für Webseite und Hotspot | 600 |
| `/etc/wpa_supplicant/wpa_supplicant.conf` | WLANs (Passwörter als Schlüssel gespeichert) | 600 |
| `/run/wordclock/state` | `client`, `connecting`, `ap` oder `error` | – |
| `/run/wordclock/ap-mode` | existiert, solange der Hotspot läuft | – |

Weitere Einstellungen in `wordclock.conf` (Änderung wirkt nach
`sudo systemctl restart wordclock-wifi`):

| Eintrag | Standard | Bedeutung |
|---------|----------|-----------|
| `wifi_interface` | `wlan0` | WLAN-Schnittstelle |
| `ap_ssid` | `WordClock-Setup` | Name des Hotspots |
| `ap_channel` | `6` | Kanal des Hotspots (1–13) |
| `connect_timeout` | `60` | Sekunden, bis ohne Verbindung der Hotspot startet |
| `ap_retry_minutes` | `10` | Minuten ohne Gerät am Hotspot, bis das WLAN erneut versucht wird |

## Sicherheit

- Der Hotspot ist mit WPA2 und dem Passwort geschützt. **Ändere das Standardpasswort**
  über die Webseite.
- Die Webseite verlangt eine Anmeldung, Formulare sind gegen fremde Seiten (CSRF) geschützt.
- Die Verbindung ist unverschlüsseltes HTTP. Für ein Heimnetz und den Hotspot reicht das,
  die Seite sollte aber nicht ins Internet freigegeben werden.
- `wordclock-web` läuft als root, weil es WLAN-Konfiguration und Systemzeit ändert.

## Fehlersuche

```bash
systemctl status wordclock-wifi wordclock-web piclock
journalctl -u wordclock-wifi -f        # was macht das WLAN gerade?
cat /run/wordclock/state
```

| Symptom | Ursache / Lösung |
|---------|------------------|
| `hostapd konnte nicht starten` | Stick kann keinen AP-Modus, oder ein anderes Programm benutzt `wlan0`. `iw list` prüfen. |
| `dnsmasq konnte nicht starten` | Ein anderer DNS-/DHCP-Server läuft (z.B. Pi-hole oder der Dienst `dnsmasq`). |
| Hotspot erscheint nicht | Ist ein bekanntes WLAN erreichbar, verbindet sich die Uhr damit. Hotspot erzwingen: WLAN auf der Webseite entfernen oder Router kurz ausschalten. |
| Handy öffnet die Seite nicht von selbst | http://10.42.0.1 von Hand im Browser öffnen. |
| `piclock.local` nicht erreichbar | Manche Android-Geräte kennen `.local` nicht: IP-Adresse aus dem Router verwenden. |
| Passwort vergessen | Per SSH `sudo nano /etc/wordclock/secret.conf`, danach `sudo systemctl restart wordclock-wifi`. |

## Testen ohne Pi

```bash
python3 web/wordclock-web --demo --port 8080      # http://localhost:8080, Passwort wordclock
python3 tests/test_web.py                         # automatische Tests
```

Im Demo-Modus wird nichts ausgeführt; Dateien landen unter `/tmp/wordclock-demo`.
