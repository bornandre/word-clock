#ifndef CONFIG_H
#define CONFIG_H

/*
 * Einstellungen der Uhr aus /etc/wordclock/wordclock.conf
 *
 * Format: eine Einstellung pro Zeile, "name = wert", Kommentare mit '#'.
 * Fehlende Datei oder fehlende Einträge -> Standardwerte.
 * Die Datei wird von der Weboberfläche geschrieben; piclock liest sie
 * automatisch neu ein, sobald sie sich ändert.
 */

#include <stdio.h>
#include <time.h>

#define CONFIG_DEFAULT_PATH "/etc/wordclock/wordclock.conf"
#define AP_FLAG_PATH        "/run/wordclock/ap-mode"   /* existiert = Hotspot aktiv */

typedef struct {
    int brightness;         /* Segmentstrom in mA: 0, 3, 6 ... 21 (0 = aus) */
    int night_enabled;      /* Nachtmodus ein/aus                           */
    int night_start;        /* Minuten seit Mitternacht                     */
    int night_end;
    int night_brightness;   /* mA im Nachtmodus                             */
} clock_config_t;

void config_defaults(clock_config_t *c);

/* Datei lesen. Rückgabe: 0 = gelesen, 1 = Datei fehlt (Standardwerte), -1 = Fehler.
 * Ungültige Zeilen werden mit Zeilennummer auf stderr gemeldet und übersprungen. */
int  config_load(const char *path, clock_config_t *c);

/* Prüft, ob sich die Datei seit dem letzten Aufruf geändert hat */
int  config_changed(const char *path, time_t *last_mtime);

/* Helligkeit (mA) zur gegebenen Uhrzeit, Nachtmodus berücksichtigt */
int  config_brightness_at(const clock_config_t *c, int hour, int minute);

/* mA -> Control-Byte des SAA1064 */
unsigned char config_control_byte(int ma);

void config_print(const clock_config_t *c, FILE *out);

#endif
