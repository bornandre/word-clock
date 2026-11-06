#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#include "config.h"
#include "saa1064.h"

static int clamp_ma(int ma) {
    if (ma < 0) ma = 0;
    if (ma > 21) ma = 21;
    return (ma / 3) * 3;                 /* nur 3-mA-Stufen möglich */
}

void config_defaults(clock_config_t *c) {
    c->brightness       = ((SAA1064_CTRL_DEFAULT >> 4) & 0x07) * 3;
    c->night_enabled    = 0;
    c->night_start      = 22 * 60;
    c->night_end        = 6 * 60 + 30;
    c->night_brightness = 3;
}

static char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    char *e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1])) *--e = '\0';
    return s;
}

static int parse_int(const char *s, int *out) {
    char *end;
    long v = strtol(s, &end, 10);
    if (*s == '\0' || *end != '\0' || v < -1000 || v > 1000) return -1;
    *out = (int)v;
    return 0;
}

static int parse_bool(const char *s, int *out) {
    if (!strcmp(s, "1") || !strcasecmp(s, "yes") || !strcasecmp(s, "true") ||
        !strcasecmp(s, "on") || !strcasecmp(s, "ja")) { *out = 1; return 0; }
    if (!strcmp(s, "0") || !strcasecmp(s, "no") || !strcasecmp(s, "false") ||
        !strcasecmp(s, "off") || !strcasecmp(s, "nein")) { *out = 0; return 0; }
    return -1;
}

static int parse_hhmm(const char *s, int *out) {
    int h, m;
    char extra;
    if (sscanf(s, "%d:%d%c", &h, &m, &extra) != 2 || h < 0 || h > 23 || m < 0 || m > 59)
        return -1;
    *out = h * 60 + m;
    return 0;
}

int config_load(const char *path, clock_config_t *c) {
    config_defaults(c);

    FILE *f = fopen(path, "r");
    if (!f) return 1;

    char line[256];
    int lineno = 0, errors = 0;
    while (fgets(line, sizeof line, f)) {
        lineno++;
        char *hash = strchr(line, '#');
        if (hash) *hash = '\0';
        char *s = trim(line);
        if (*s == '\0') continue;

        char *eq = strchr(s, '=');
        if (!eq) {
            fprintf(stderr, "%s:%d: '=' fehlt\n", path, lineno);
            errors++;
            continue;
        }
        *eq = '\0';
        char *key = trim(s), *val = trim(eq + 1);
        int v, rc = 0;

        if (!strcmp(key, "brightness")) {
            if ((rc = parse_int(val, &v)) == 0) c->brightness = clamp_ma(v);
        } else if (!strcmp(key, "night_enabled")) {
            rc = parse_bool(val, &c->night_enabled);
        } else if (!strcmp(key, "night_start")) {
            rc = parse_hhmm(val, &c->night_start);
        } else if (!strcmp(key, "night_end")) {
            rc = parse_hhmm(val, &c->night_end);
        } else if (!strcmp(key, "night_brightness")) {
            if ((rc = parse_int(val, &v)) == 0) c->night_brightness = clamp_ma(v);
        } else {
            continue;          /* unbekannte Einträge gehören anderen Programmen */
        }
        if (rc != 0) {
            fprintf(stderr, "%s:%d: ungültiger Wert für %s: %s\n", path, lineno, key, val);
            errors++;
        }
    }
    fclose(f);
    return errors ? -1 : 0;
}

int config_changed(const char *path, time_t *last_mtime) {
    struct stat st;
    time_t m = (stat(path, &st) == 0) ? st.st_mtime : 0;
    if (m != *last_mtime) {
        *last_mtime = m;
        return 1;
    }
    return 0;
}

int config_brightness_at(const clock_config_t *c, int hour, int minute) {
    if (!c->night_enabled || c->night_start == c->night_end) return c->brightness;
    int t = hour * 60 + minute;
    int night = (c->night_start < c->night_end)
                    ? (t >= c->night_start && t < c->night_end)      /* z.B. 01:00–05:00 */
                    : (t >= c->night_start || t < c->night_end);     /* über Mitternacht  */
    return night ? c->night_brightness : c->brightness;
}

unsigned char config_control_byte(int ma) {
    return (unsigned char)(SAA1064_CTRL_DYNAMIC | SAA1064_CTRL_DIG13_ON |
                           SAA1064_CTRL_DIG24_ON | ((clamp_ma(ma) / 3) << 4));
}

void config_print(const clock_config_t *c, FILE *out) {
    fprintf(out, "brightness=%d night_enabled=%d night_start=%02d:%02d "
                 "night_end=%02d:%02d night_brightness=%d\n",
            c->brightness, c->night_enabled,
            c->night_start / 60, c->night_start % 60,
            c->night_end / 60, c->night_end % 60, c->night_brightness);
}
