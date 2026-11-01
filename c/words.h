#ifndef WORDS_H
#define WORDS_H

#include <stddef.h>
#include <stdint.h>

#define NUM_CHIPS  4
#define NUM_DIGITS 4

/* Index der Chips im LED-Puffer */
enum { CHIP_38 = 0, CHIP_39 = 1, CHIP_3A = 2, CHIP_3B = 3 };

extern const uint8_t chip_addr[NUM_CHIPS];   /* 0x38, 0x39, 0x3a, 0x3b */

/* LED-Puffer: d[chip][digit] = Segment-Bitmaske
 * (entspricht den bisherigen Arrays a38[], a39[], a3a[], a3b[]) */
typedef struct {
    uint8_t d[NUM_CHIPS][NUM_DIGITS];
} ledbuf_t;

typedef enum {
    W_ES, W_ISCH,
    /* Minuten */
    W_FUEF, W_ZAEAE, W_VIERTU, W_ZWAENZG, W_AB, W_VOR, W_HAUBI,
    /* Stunden */
    W_EIS, W_ZWOEI, W_DRUE, W_VIERI, W_FUEFI, W_SAECHSI,
    W_SIBNI, W_ACHTI, W_NUENI, W_ZAENI, W_ELFI, W_ZWOEUFI,
    /* Minutenpunkte */
    W_PUNKT1, W_PUNKT2, W_PUNKT3, W_PUNKT4,
    W_COUNT
} word_id_t;

void        ledbuf_clear(ledbuf_t *b);
void        set_word(ledbuf_t *b, word_id_t w);       /* Wort hinzufügen (ODER) */
const char *word_name(word_id_t w);

/* Komplette Uhrzeit in den Puffer schreiben.
 * text (optional, darf NULL sein) erhält den angezeigten Satz. */
void build_time(ledbuf_t *b, int hour, int minute, char *text, size_t len);

/* Puffer an alle 4 SAA1064 senden. Rückgabe: Anzahl nicht erreichbarer Chips */
int  write_buffers(const ledbuf_t *b);

/* Prüft, ob sich zwei Wörter dieselben LEDs teilen. Rückgabe: Anzahl Konflikte */
int  words_check_overlaps(void);

/* Alle LEDs setzen, die von irgendeinem Wort benutzt werden */
void words_used(ledbuf_t *b);

#endif
