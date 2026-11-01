/*
 * test_leds – Hilfsprogramm zum Prüfen der LED-Zuordnung
 *
 *   ./test_leds            alle 128 LEDs einzeln nacheinander (Endlosschleife)
 *   ./test_leds -d 200     Verzögerung in ms (Standard 500)
 *   ./test_leds -m         manuell: Enter = weiter, z = zurück, Zahl = springen, q = Ende
 *   ./test_leds -s 40      bei LED 40 beginnen
 *   ./test_leds -w         alle definierten Wörter einzeln anzeigen (mit -m manuell)
 *   ./test_leds -a         alle LEDs an
 *   ./test_leds -c         Wort-Tabelle auf Überschneidungen prüfen (ohne I2C)
 *   ./test_leds -f [-m]    nur LEDs durchgehen, die noch KEINEM Wort zugeordnet sind
 *   ./test_leds -l         freie LEDs nur auflisten (ohne I2C)
 *
 * LED-Nummer = Chip * 32 + Digit * 8 + Segment
 * Ausgabe z.B. "LED  77  ->  a3a[1] = 0x20"  (Schreibweise wie im Code)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include "i2c.h"
#include "saa1064.h"
#include "words.h"

#define NUM_LEDS (NUM_CHIPS * NUM_DIGITS * 8)

static volatile sig_atomic_t keep_running = 1;

static void on_signal(int sig) {
    (void)sig;
    keep_running = 0;
}

static void sleep_ms(long ms) {
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };
    while (nanosleep(&ts, &ts) == -1 && keep_running) { }
}

static void describe(int nr) {
    int chip = nr / 32, digit = (nr % 32) / 8, seg = nr % 8;
    printf("LED %3d  ->  a%02x[%d] = 0x%02X", nr, chip_addr[chip], digit, 1u << seg);
}

static void show_single(int nr) {
    ledbuf_t b;
    ledbuf_clear(&b);
    b.d[nr / 32][(nr % 32) / 8] = (uint8_t)(1u << (nr % 8));
    write_buffers(&b);
}

/* Eingabe im manuellen Modus: liefert neue Position oder -1 für Ende */
static int ask(int pos, int count) {
    char line[32];
    printf("  > ");
    fflush(stdout);
    if (!fgets(line, sizeof line, stdin)) return -1;   /* EOF / Strg+C */
    char *p = line;
    while (isspace((unsigned char)*p)) p++;
    if (*p == 'q') return -1;
    if (*p == 'z') return (pos - 1 + count) % count;
    if (isdigit((unsigned char)*p)) return atoi(p) % count;
    return (pos + 1) % count;
}

static void run_leds(int start, int manual, long delay) {
    int nr = start % NUM_LEDS;
    while (keep_running) {
        show_single(nr);
        describe(nr);
        if (manual) {
            nr = ask(nr, NUM_LEDS);
            if (nr < 0) break;
        } else {
            printf("\n");
            fflush(stdout);
            sleep_ms(delay);
            nr = (nr + 1) % NUM_LEDS;
        }
    }
}

/* Liste der LEDs, die von keinem Wort benutzt werden */
static int free_leds(int *list) {
    ledbuf_t used;
    words_used(&used);
    int n = 0;
    for (int nr = 0; nr < NUM_LEDS; nr++) {
        if (!(used.d[nr / 32][(nr % 32) / 8] & (1u << (nr % 8)))) list[n++] = nr;
    }
    return n;
}

static void print_free(const int *list, int n) {
    printf("%d freie LEDs:\n", n);
    for (int i = 0; i < n; i++) {
        printf("  ");
        describe(list[i]);
        printf("\n");
    }
}

static void run_free(int manual, long delay) {
    int list[NUM_LEDS];
    int n = free_leds(list);
    if (n == 0) { printf("Keine freien LEDs.\n"); return; }
    printf("%d freie LEDs (Zahl = Position in dieser Liste)\n", n);
    int i = 0;
    while (keep_running) {
        show_single(list[i]);
        printf("[%2d/%d] ", i, n);
        describe(list[i]);
        if (manual) {
            i = ask(i, n);
            if (i < 0) break;
        } else {
            printf("\n");
            fflush(stdout);
            sleep_ms(delay * 2);
            i = (i + 1) % n;
        }
    }
}

static void run_words(int manual, long delay) {
    int w = 0;
    while (keep_running) {
        ledbuf_t b;
        ledbuf_clear(&b);
        set_word(&b, (word_id_t)w);
        write_buffers(&b);
        printf("Wort %2d: %-8s", w, word_name((word_id_t)w));
        if (manual) {
            w = ask(w, W_COUNT);
            if (w < 0) break;
        } else {
            printf("\n");
            fflush(stdout);
            sleep_ms(delay * 3);
            w = (w + 1) % W_COUNT;
        }
    }
}

int main(int argc, char **argv) {
    long delay = 500;
    int manual = 0, words_mode = 0, all_on = 0, check = 0, start = 0;
    int opt;

    int free_mode = 0;
    while ((opt = getopt(argc, argv, "d:ms:wacflh")) != -1) {
        switch (opt) {
        case 'd': delay = atol(optarg); if (delay < 10) delay = 10; break;
        case 'm': manual = 1;     break;
        case 's': start = atoi(optarg); if (start < 0) start = 0; break;
        case 'w': words_mode = 1; break;
        case 'a': all_on = 1;     break;
        case 'c': check = 1;      break;
        case 'f': free_mode = 1;  break;
        case 'l': {
            int list[NUM_LEDS];
            print_free(list, free_leds(list));
            return 0;
        }
        default:
            fprintf(stderr, "Verwendung: %s [-d ms] [-m] [-s nr] [-w] [-a] [-c] [-f] [-l]\n", argv[0]);
            return opt == 'h' ? 0 : 1;
        }
    }

    if (check) {
        int n = words_check_overlaps();
        printf(n ? "%d Konflikt(e) gefunden\n" : "Keine Überschneidungen\n", n);
        return n ? 2 : 0;
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_signal;          /* ohne SA_RESTART: fgets bricht bei Strg+C ab */
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    if (i2c_open_bus() != 0) return 1;

    int found = 0;
    for (int c = 0; c < NUM_CHIPS; c++) {
        if (saa1064_init(chip_addr[c]) == 0) {
            printf("SAA1064 gefunden auf 0x%02x\n", chip_addr[c]);
            found++;
        } else {
            printf("WARNUNG: kein SAA1064 auf 0x%02x\n", chip_addr[c]);
        }
    }
    if (!found) {
        fprintf(stderr, "Kein SAA1064 erreichbar – Verdrahtung / i2cdetect -y 1 prüfen\n");
        i2c_close_bus();
        return 1;
    }

    if (all_on) {
        ledbuf_t b;
        memset(&b, 0xFF, sizeof b);
        write_buffers(&b);
        printf("Alle LEDs an – Strg+C zum Beenden\n");
        while (keep_running) sleep_ms(500);
    } else if (free_mode) {
        run_free(manual, delay);
    } else if (words_mode) {
        run_words(manual, delay);
    } else {
        run_leds(start, manual, delay);
    }

    printf("\nAlle LEDs aus.\n");
    for (int c = 0; c < NUM_CHIPS; c++) saa1064_clear(chip_addr[c]);
    i2c_close_bus();
    return 0;
}
