# Frontplatte und LED-Zuordnung

## Buchstabenraster (10 × 11, Schweizerdeutsch)

```
E S K I S C H A F Ü F
V I E R T U B F Z Ä Ä
Z W Ä N Z G S I V O R
A B O H A U B I E P M
E I S Z W Ö I S D R Ü
V I E R I F Ü F I Q T
S Ä C H S I S I B N I
A C H T I N Ü N I E L
Z Ä N I E R B E L F I
Z W Ö U F I A M U H R
```

Dazu kommen 4 Minutenpunkte für die Minuten zwischen den 5-Minuten-Schritten.

## Verdrahtung

Jede Zeile ist gleich aufgebaut: Die **ersten 3 Buchstaben** hängen am Chip `0x3B`,
die **restlichen 8** an einem Digit eines anderen Chips. Innerhalb eines Digits
steht das höchste Bit (`0x80`) links.

| Zeile | Buchstaben 1–3          | Buchstaben 4–11 |
|------:|-------------------------|-----------------|
| 1     | `3b[0]` 0x04 0x02 0x01  | `39[0]`         |
| 2     | `3b[0]` 0x20 0x10 0x08  | `39[2]`         |
| 3     | `3b[0]` 0x40, `3b[2]` 0x02 0x01 | `39[1]` |
| 4     | `3b[2]` 0x10 0x08 0x04  | `39[3]`         |
| 5     | `3b[2]` 0x80 0x40 0x20  | `3a[3]`         |
| 6     | `3b[1]` 0x04 0x02 0x01  | `3a[1]`         |
| 7     | `3b[1]` 0x20 0x10 0x08  | `3a[2]`         |
| 8     | `3b[1]` 0x40, `3b[3]` 0x02 0x01 | `3a[0]` |
| 9     | `3b[3]` 0x10 0x08 0x04  | `38[2]`         |
| 10    | `3b[3]` 0x80 0x40 0x20  | `38[0]`         |

**Minutenpunkte:** 1 = `3b[0]` 0x80 · 2 = `38[3]` 0x01 · 3 = `38[1]` 0x01 · 4 = `3b[1]` 0x80

**Nicht belegt:** `38[1]` und `38[3]`, jeweils Bits 0x02–0x80 (14 freie Ausgänge).

Schreibweise: `3a[1]` 0x20 bedeutet Chip `0x3A`, Digit 2 (Index 1), Segment-Bit `0x20`.
Insgesamt 114 LEDs (110 Buchstaben + 4 Punkte) an 4 × SAA1064 im Multiplex-Betrieb.

## Jeder Buchstabe einzeln

| Zeile | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | **E**<br>`3b[0]`<br>`0x04` | **S**<br>`3b[0]`<br>`0x02` | **K**<br>`3b[0]`<br>`0x01` | **I**<br>`39[0]`<br>`0x80` | **S**<br>`39[0]`<br>`0x40` | **C**<br>`39[0]`<br>`0x20` | **H**<br>`39[0]`<br>`0x10` | **A**<br>`39[0]`<br>`0x08` | **F**<br>`39[0]`<br>`0x04` | **Ü**<br>`39[0]`<br>`0x02` | **F**<br>`39[0]`<br>`0x01` |
| 2 | **V**<br>`3b[0]`<br>`0x20` | **I**<br>`3b[0]`<br>`0x10` | **E**<br>`3b[0]`<br>`0x08` | **R**<br>`39[2]`<br>`0x80` | **T**<br>`39[2]`<br>`0x40` | **U**<br>`39[2]`<br>`0x20` | **B**<br>`39[2]`<br>`0x10` | **F**<br>`39[2]`<br>`0x08` | **Z**<br>`39[2]`<br>`0x04` | **Ä**<br>`39[2]`<br>`0x02` | **Ä**<br>`39[2]`<br>`0x01` |
| 3 | **Z**<br>`3b[0]`<br>`0x40` | **W**<br>`3b[2]`<br>`0x02` | **Ä**<br>`3b[2]`<br>`0x01` | **N**<br>`39[1]`<br>`0x80` | **Z**<br>`39[1]`<br>`0x40` | **G**<br>`39[1]`<br>`0x20` | **S**<br>`39[1]`<br>`0x10` | **I**<br>`39[1]`<br>`0x08` | **V**<br>`39[1]`<br>`0x04` | **O**<br>`39[1]`<br>`0x02` | **R**<br>`39[1]`<br>`0x01` |
| 4 | **A**<br>`3b[2]`<br>`0x10` | **B**<br>`3b[2]`<br>`0x08` | **O**<br>`3b[2]`<br>`0x04` | **H**<br>`39[3]`<br>`0x80` | **A**<br>`39[3]`<br>`0x40` | **U**<br>`39[3]`<br>`0x20` | **B**<br>`39[3]`<br>`0x10` | **I**<br>`39[3]`<br>`0x08` | **E**<br>`39[3]`<br>`0x04` | **P**<br>`39[3]`<br>`0x02` | **M**<br>`39[3]`<br>`0x01` |
| 5 | **E**<br>`3b[2]`<br>`0x80` | **I**<br>`3b[2]`<br>`0x40` | **S**<br>`3b[2]`<br>`0x20` | **Z**<br>`3a[3]`<br>`0x80` | **W**<br>`3a[3]`<br>`0x40` | **Ö**<br>`3a[3]`<br>`0x20` | **I**<br>`3a[3]`<br>`0x10` | **S**<br>`3a[3]`<br>`0x08` | **D**<br>`3a[3]`<br>`0x04` | **R**<br>`3a[3]`<br>`0x02` | **Ü**<br>`3a[3]`<br>`0x01` |
| 6 | **V**<br>`3b[1]`<br>`0x04` | **I**<br>`3b[1]`<br>`0x02` | **E**<br>`3b[1]`<br>`0x01` | **R**<br>`3a[1]`<br>`0x80` | **I**<br>`3a[1]`<br>`0x40` | **F**<br>`3a[1]`<br>`0x20` | **Ü**<br>`3a[1]`<br>`0x10` | **F**<br>`3a[1]`<br>`0x08` | **I**<br>`3a[1]`<br>`0x04` | **Q**<br>`3a[1]`<br>`0x02` | **T**<br>`3a[1]`<br>`0x01` |
| 7 | **S**<br>`3b[1]`<br>`0x20` | **Ä**<br>`3b[1]`<br>`0x10` | **C**<br>`3b[1]`<br>`0x08` | **H**<br>`3a[2]`<br>`0x80` | **S**<br>`3a[2]`<br>`0x40` | **I**<br>`3a[2]`<br>`0x20` | **S**<br>`3a[2]`<br>`0x10` | **I**<br>`3a[2]`<br>`0x08` | **B**<br>`3a[2]`<br>`0x04` | **N**<br>`3a[2]`<br>`0x02` | **I**<br>`3a[2]`<br>`0x01` |
| 8 | **A**<br>`3b[1]`<br>`0x40` | **C**<br>`3b[3]`<br>`0x02` | **H**<br>`3b[3]`<br>`0x01` | **T**<br>`3a[0]`<br>`0x80` | **I**<br>`3a[0]`<br>`0x40` | **N**<br>`3a[0]`<br>`0x20` | **Ü**<br>`3a[0]`<br>`0x10` | **N**<br>`3a[0]`<br>`0x08` | **I**<br>`3a[0]`<br>`0x04` | **E**<br>`3a[0]`<br>`0x02` | **L**<br>`3a[0]`<br>`0x01` |
| 9 | **Z**<br>`3b[3]`<br>`0x10` | **Ä**<br>`3b[3]`<br>`0x08` | **N**<br>`3b[3]`<br>`0x04` | **I**<br>`38[2]`<br>`0x80` | **E**<br>`38[2]`<br>`0x40` | **R**<br>`38[2]`<br>`0x20` | **B**<br>`38[2]`<br>`0x10` | **E**<br>`38[2]`<br>`0x08` | **L**<br>`38[2]`<br>`0x04` | **F**<br>`38[2]`<br>`0x02` | **I**<br>`38[2]`<br>`0x01` |
| 10 | **Z**<br>`3b[3]`<br>`0x80` | **W**<br>`3b[3]`<br>`0x40` | **Ö**<br>`3b[3]`<br>`0x20` | **U**<br>`38[0]`<br>`0x80` | **F**<br>`38[0]`<br>`0x40` | **I**<br>`38[0]`<br>`0x20` | **A**<br>`38[0]`<br>`0x10` | **M**<br>`38[0]`<br>`0x08` | **U**<br>`38[0]`<br>`0x04` | **H**<br>`38[0]`<br>`0x02` | **R**<br>`38[0]`<br>`0x01` |

## Wörter

| Wort | Zeile | Verwendung |
|------|------:|------------|
| ES ISCH | 1 | immer |
| FÜF / ZÄÄ / VIERTU / ZWÄNZG | 1–3 | Minuten |
| AB / VOR / HAUBI | 3–4 | Minuten |
| EIS … ZWÖUFI | 5–10 | Stunden |
| AM, UHR | 10 | frei (nicht verwendet) |

Ab :25 bezieht sich die Anzeige auf die **nächste** Stunde
(14:25 → «ES ISCH FÜF VOR HAUBI DRÜ»).

Diese Datei wird teilweise erzeugt mit `python3 tools/check_layout.py --table`.
Wer die Verdrahtung ändert, passt `ROWS` in `tools/check_layout.py` und die Tabelle
in `c/words.c` an; `make check` und `python3 tools/check_layout.py` prüfen beides.
