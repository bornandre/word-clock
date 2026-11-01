/*
 * piclock – Schweizerdeutsche Wortuhr mit 4x SAA1064 am Raspberry Pi
 *
 *   ./piclock            normale Uhr
 *   ./piclock -t 14:25   feste Zeit anzeigen (bis Strg+C)
 *   ./piclock -d         Demo: alle 5-Minuten-Schritte eines halben Tages
 *   ./piclock -n -d      Trockenlauf ohne I2C (nur Text), z.B. auf dem PC
 *   ./piclock -n -a      alle 1440 Minuten eines Tages ausgeben (Regressionstest)
 *   ./piclock -v         jede Minute den angezeigten Satz ausgeben
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include "i2c.h"
#include "saa1064.h"
#include "words.h"

static volatile sig_atomic_t keep_running = 1;

static void on_signal(int sig) {
    (void)sig;
    keep_running = 0;
}

static void sleep_ms(long ms) {
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };
    /* bei Signal sofort abbrechen */
    while (nanosleep(&ts, &ts) == -1 && keep_running) { }
}

static void usage(const char *prog) {
    fprintf(stderr,
            "Verwendung: %s [Optionen]\n"
            "  -t HH:MM  feste Uhrzeit anzeigen (Test)\n"
            "  -d        Demo: alle 5-Minuten-Schritte durchlaufen\n"
            "  -a        alle Minuten eines Tages ausgeben (mit -n, für Tests)\n"
            "  -n        Trockenlauf ohne I2C (nur Textausgabe)\n"
            "  -v        angezeigten Satz bei jeder Änderung ausgeben\n"
            "  -h        diese Hilfe\n", prog);
}

/* Puffer senden; Fehler nur bei Zustandswechsel melden (kein Log-Spam) */
static void show(const ledbuf_t *buf, int dry, int *last_err) {
    if (dry) return;
    int err = write_buffers(buf);
    if (err && err != *last_err)
        fprintf(stderr, "WARNUNG: %d SAA1064 nicht erreichbar\n", err);
    else if (!err && *last_err)
        fprintf(stderr, "I2C wieder in Ordnung\n");
    *last_err = err;
}

int main(int argc, char **argv) {
    int fixed_h = -1, fixed_m = -1;
    int demo = 0, dry = 0, verbose = 0, all_minutes = 0;
    int opt;

    while ((opt = getopt(argc, argv, "t:dnavh")) != -1) {
        switch (opt) {
        case 't':
            if (sscanf(optarg, "%d:%d", &fixed_h, &fixed_m) != 2 ||
                fixed_h < 0 || fixed_h > 23 || fixed_m < 0 || fixed_m > 59) {
                fprintf(stderr, "Ungültige Zeit: %s (Format HH:MM)\n", optarg);
                return 1;
            }
            break;
        case 'd': demo = 1;    break;
        case 'n': dry = 1;     break;
        case 'a': all_minutes = 1; demo = 1; break;
        case 'v': verbose = 1; break;
        case 'h': usage(argv[0]); return 0;
        default:  usage(argv[0]); return 1;
        }
    }
    if (dry) verbose = 1;

    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    if (!dry) {
        if (i2c_open_bus() != 0) return 1;
        for (int c = 0; c < NUM_CHIPS; c++) {
            if (saa1064_init(chip_addr[c]) != 0)
                fprintf(stderr, "WARNUNG: kein SAA1064 auf 0x%02x\n", chip_addr[c]);
        }
    }

    ledbuf_t buf;
    char text[128];
    int last_err = 0;

    if (demo) {
        int step = all_minutes ? 1 : 5;
        int end  = all_minutes ? 24 * 60 : 12 * 60;
        for (int t = 0; t < end && keep_running; t += step) {
            build_time(&buf, t / 60, t % 60, text, sizeof text);
            printf("%02d:%02d  %s\n", t / 60, t % 60, text);
            fflush(stdout);
            show(&buf, dry, &last_err);
            sleep_ms(dry ? 0 : 700);
        }
    } else if (fixed_h >= 0) {
        build_time(&buf, fixed_h, fixed_m, text, sizeof text);
        printf("%02d:%02d  %s\n", fixed_h, fixed_m, text);
        fflush(stdout);
        show(&buf, dry, &last_err);
        if (!dry) {
            printf("Anzeige bleibt an – Strg+C zum Beenden\n");
            while (keep_running) sleep_ms(1000);
        }
    } else {
        int last_h = -1, last_m = -1;
        time_t last_write = 0;

        while (keep_running) {
            time_t now = time(NULL);
            struct tm tmv;
            if (localtime_r(&now, &tmv) == NULL) {
                sleep_ms(1000);
                continue;
            }

            int changed = (tmv.tm_hour != last_h || tmv.tm_min != last_m);

            /* bei Minutenwechsel sofort, sonst alle 10 s auffrischen
             * (falls ein Chip zwischenzeitlich zurückgesetzt wurde) */
            if (changed || now - last_write >= 10) {
                build_time(&buf, tmv.tm_hour, tmv.tm_min, text, sizeof text);
                if (changed && verbose) {
                    printf("%02d:%02d  %s\n", tmv.tm_hour, tmv.tm_min, text);
                    fflush(stdout);
                }
                show(&buf, dry, &last_err);
                last_h = tmv.tm_hour;
                last_m = tmv.tm_min;
                last_write = now;
            }
            sleep_ms(1000);
        }
    }

    if (!dry) {
        for (int c = 0; c < NUM_CHIPS; c++) saa1064_clear(chip_addr[c]);
        i2c_close_bus();
    }
    return 0;
}
