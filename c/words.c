#include <stdio.h>
#include <string.h>
#include "saa1064.h"
#include "words.h"

const uint8_t chip_addr[NUM_CHIPS] = { 0x38, 0x39, 0x3a, 0x3b };

#define MAX_PARTS 3

typedef struct {
    uint8_t chip;    /* CHIP_38 .. CHIP_3B         */
    uint8_t digit;   /* 0..3  (= Index im Array)   */
    uint8_t mask;    /* Segment-Bits               */
} led_part_t;

typedef struct {
    const char *name;
    uint8_t     nparts;
    led_part_t  part[MAX_PARTS];
} word_def_t;

#define P(c, d, m) { (c), (d), (m) }

/*
 * Zentrale Wort-Tabelle – auf der Hardware verifiziert.
 * Raster und Verdrahtung: docs/layout.md, Prüfung: tools/check_layout.py
 * Schreibweise im Kommentar: a3b[0] |= 0x06 usw.
 *
 * ZÄNI ist auf der Hardware bestätigt: a3b[3] 0x18 = "ZÄ", 0x04 = "N", a38[2] 0x80 = "I".
 * AB liegt auf a3b[2]: 0x10 = "A" (LED 116), 0x08 = "B" (LED 115)  – Zeile "ABO HAUBI EPM".
 *
 * Füllbuchstaben (keinem Wort zugeordnet, laut "test_leds -f"):
 *   a38[0] 0x10 A, 0x08 M, 0x04 U, 0x02 H, 0x01 R   a38[2] 0x40 E, 0x20 R, 0x10 B
 *   a39[0] 0x08 A   a39[1] 0x10 S, 0x08 I   a39[2] 0x10 B, 0x08 F
 *   a39[3] 0x04 E, 0x02 P, 0x01 M   a3a[0] 0x02 E, 0x01 L   a3a[1] 0x02 Q, 0x01 T
 *   a3a[3] 0x08 S   a3b[0] 0x01 K   a3b[2] 0x04 O
 *   a38[1] 0x02..0x80 und a38[3] 0x02..0x80: nicht bestückt
 */
static const word_def_t words[W_COUNT] = {
    [W_ES]      = { "ES",     1, { P(CHIP_3B, 0, 0x06) } },
    [W_ISCH]    = { "ISCH",   1, { P(CHIP_39, 0, 0xF0) } },

    [W_FUEF]    = { "FÜF",    1, { P(CHIP_39, 0, 0x07) } },
    [W_ZAEAE]   = { "ZÄÄ",    1, { P(CHIP_39, 2, 0x07) } },
    [W_VIERTU]  = { "VIERTU", 2, { P(CHIP_3B, 0, 0x38), P(CHIP_39, 2, 0xE0) } },
    [W_ZWAENZG] = { "ZWÄNZG", 3, { P(CHIP_3B, 0, 0x40), P(CHIP_3B, 2, 0x03),
                                   P(CHIP_39, 1, 0xE0) } },
    [W_AB]      = { "AB",     1, { P(CHIP_3B, 2, 0x18) } },
    [W_VOR]     = { "VOR",    1, { P(CHIP_39, 1, 0x07) } },
    [W_HAUBI]   = { "HAUBI",  1, { P(CHIP_39, 3, 0xF8) } },

    [W_EIS]     = { "EIS",    1, { P(CHIP_3B, 2, 0xE0) } },
    [W_ZWOEI]   = { "ZWÖI",   1, { P(CHIP_3A, 3, 0xF0) } },
    [W_DRUE]    = { "DRÜ",    1, { P(CHIP_3A, 3, 0x07) } },
    [W_VIERI]   = { "VIERI",  2, { P(CHIP_3B, 1, 0x07), P(CHIP_3A, 1, 0xC0) } },
    [W_FUEFI]   = { "FÜFI",   1, { P(CHIP_3A, 1, 0x3C) } },
    [W_SAECHSI] = { "SÄCHSI", 2, { P(CHIP_3B, 1, 0x38), P(CHIP_3A, 2, 0xE0) } },
    [W_SIBNI]   = { "SIBNI",  1, { P(CHIP_3A, 2, 0x1F) } },
    [W_ACHTI]   = { "ACHTI",  3, { P(CHIP_3B, 3, 0x03), P(CHIP_3A, 0, 0xC0),
                                   P(CHIP_3B, 1, 0x40) } },
    [W_NUENI]   = { "NÜNI",   1, { P(CHIP_3A, 0, 0x3C) } },
    [W_ZAENI]   = { "ZÄNI",   2, { P(CHIP_3B, 3, 0x1C), P(CHIP_38, 2, 0x80) } },
    [W_ELFI]    = { "ELFI",   1, { P(CHIP_38, 2, 0x0F) } },
    [W_ZWOEUFI] = { "ZWÖUFI", 2, { P(CHIP_3B, 3, 0xE0), P(CHIP_38, 0, 0xE0) } },

    [W_PUNKT1]  = { "•1",     1, { P(CHIP_3B, 0, 0x80) } },
    [W_PUNKT2]  = { "•2",     1, { P(CHIP_38, 3, 0x01) } },
    [W_PUNKT3]  = { "•3",     1, { P(CHIP_38, 1, 0x01) } },
    [W_PUNKT4]  = { "•4",     1, { P(CHIP_3B, 1, 0x80) } },
};

/* Stundenwörter, Index = Stunde % 12 */
static const word_id_t hour_word[12] = {
    W_ZWOEUFI, W_EIS, W_ZWOEI, W_DRUE, W_VIERI, W_FUEFI,
    W_SAECHSI, W_SIBNI, W_ACHTI, W_NUENI, W_ZAENI, W_ELFI
};

/* Minutenwörter je 5-Minuten-Schritt (W_COUNT = Ende der Liste) */
static const word_id_t minute_words[12][4] = {
    /* :00 */ { W_COUNT },
    /* :05 */ { W_FUEF,    W_AB,    W_COUNT },
    /* :10 */ { W_ZAEAE,   W_AB,    W_COUNT },
    /* :15 */ { W_VIERTU,  W_AB,    W_COUNT },
    /* :20 */ { W_ZWAENZG, W_AB,    W_COUNT },
    /* :25 */ { W_FUEF,    W_VOR,   W_HAUBI, W_COUNT },
    /* :30 */ { W_HAUBI,   W_COUNT },
    /* :35 */ { W_FUEF,    W_AB,    W_HAUBI, W_COUNT },
    /* :40 */ { W_ZWAENZG, W_VOR,   W_COUNT },
    /* :45 */ { W_VIERTU,  W_VOR,   W_COUNT },
    /* :50 */ { W_ZAEAE,   W_VOR,   W_COUNT },
    /* :55 */ { W_FUEF,    W_VOR,   W_COUNT },
};

void ledbuf_clear(ledbuf_t *b) {
    memset(b, 0, sizeof *b);
}

void set_word(ledbuf_t *b, word_id_t w) {
    if ((int)w < 0 || w >= W_COUNT) return;
    const word_def_t *wd = &words[w];
    for (int i = 0; i < wd->nparts; i++) {
        b->d[wd->part[i].chip][wd->part[i].digit] |= wd->part[i].mask;
    }
}

void clear_word(ledbuf_t *b, word_id_t w) {
    if ((int)w < 0 || w >= W_COUNT) return;
    const word_def_t *wd = &words[w];
    for (int i = 0; i < wd->nparts; i++) {
        b->d[wd->part[i].chip][wd->part[i].digit] &= (uint8_t)~wd->part[i].mask;
    }
}

const char *word_name(word_id_t w) {
    if ((int)w < 0 || w >= W_COUNT) return "?";
    return words[w].name;
}

static void text_add(char *text, size_t len, const char *s) {
    if (!text || len == 0) return;
    size_t used = strlen(text);
    snprintf(text + used, len - used, "%s%s", used ? " " : "", s);
}

void build_time(ledbuf_t *b, int hour, int minute, char *text, size_t len) {
    if (text && len) text[0] = '\0';
    ledbuf_clear(b);

    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) return;

    int step = minute / 5;   /* 0..11 */
    int dots = minute % 5;   /* 0..4  */

    /* Ab :25 bezieht sich die Zeit auf die nächste Stunde:
     * 14:25 = "FÜF VOR HAUBI DRÜ" */
    if (step >= 5) hour++;

    set_word(b, W_ES);   text_add(text, len, word_name(W_ES));
    set_word(b, W_ISCH); text_add(text, len, word_name(W_ISCH));

    for (int i = 0; minute_words[step][i] != W_COUNT; i++) {
        set_word(b, minute_words[step][i]);
        text_add(text, len, word_name(minute_words[step][i]));
    }

    word_id_t hw = hour_word[hour % 12];
    set_word(b, hw);
    text_add(text, len, word_name(hw));

    for (int i = 0; i < dots; i++) {
        set_word(b, (word_id_t)(W_PUNKT1 + i));
    }
    if (dots && text && len) {
        char tmp[8];
        snprintf(tmp, sizeof tmp, "(+%d)", dots);
        text_add(text, len, tmp);
    }
}

int write_buffers(const ledbuf_t *b) {
    int errors = 0;
    for (int c = 0; c < NUM_CHIPS; c++) {
        if (saa1064_write_digits(chip_addr[c],
                                 b->d[c][0], b->d[c][1],
                                 b->d[c][2], b->d[c][3]) != 0) {
            errors++;
        }
    }
    return errors;
}

void words_used(ledbuf_t *b) {
    ledbuf_clear(b);
    for (int w = 0; w < W_COUNT; w++) set_word(b, (word_id_t)w);
}

int words_check_overlaps(void) {
    int conflicts = 0;
    for (int i = 0; i < W_COUNT; i++) {
        for (int j = i + 1; j < W_COUNT; j++) {
            for (int pi = 0; pi < words[i].nparts; pi++) {
                for (int pj = 0; pj < words[j].nparts; pj++) {
                    const led_part_t *a = &words[i].part[pi];
                    const led_part_t *c = &words[j].part[pj];
                    if (a->chip == c->chip && a->digit == c->digit &&
                        (a->mask & c->mask)) {
                        printf("KONFLIKT: %-7s und %-7s teilen a%02x[%d] Bits 0x%02X\n",
                               words[i].name, words[j].name,
                               chip_addr[a->chip], a->digit, a->mask & c->mask);
                        conflicts++;
                    }
                }
            }
        }
    }
    return conflicts;
}
