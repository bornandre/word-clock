#!/usr/bin/env python3
"""
Prüft die Wort-Tabelle in c/words.c gegen das Buchstabenraster der Frontplatte
und erzeugt auf Wunsch die Zuordnungstabelle für docs/layout.md.

    python3 tools/check_layout.py            # Prüfung (Exit-Code 1 bei Fehler)
    python3 tools/check_layout.py --table    # Markdown-Tabelle ausgeben
"""
import re
import sys
from pathlib import Path

GRID = [
    "ESKISCHAFÜF",
    "VIERTUBFZÄÄ",
    "ZWÄNZGSIVOR",
    "ABOHAUBIEPM",
    "EISZWÖISDRÜ",
    "VIERIFÜFIQT",
    "SÄCHSISIBNI",
    "ACHTINÜNIEL",
    "ZÄNIERBELFI",
    "ZWÖUFIAMUHR",
]

# Verdrahtung je Zeile, von links nach rechts: (Chip, Digit, [Bits])
ROWS = [
    [(0x3B, 0, [2, 1, 0]),        (0x39, 0, range(7, -1, -1))],
    [(0x3B, 0, [5, 4, 3]),        (0x39, 2, range(7, -1, -1))],
    [(0x3B, 0, [6]), (0x3B, 2, [1, 0]), (0x39, 1, range(7, -1, -1))],
    [(0x3B, 2, [4, 3, 2]),        (0x39, 3, range(7, -1, -1))],
    [(0x3B, 2, [7, 6, 5]),        (0x3A, 3, range(7, -1, -1))],
    [(0x3B, 1, [2, 1, 0]),        (0x3A, 1, range(7, -1, -1))],
    [(0x3B, 1, [5, 4, 3]),        (0x3A, 2, range(7, -1, -1))],
    [(0x3B, 1, [6]), (0x3B, 3, [1, 0]), (0x3A, 0, range(7, -1, -1))],
    [(0x3B, 3, [4, 3, 2]),        (0x38, 2, range(7, -1, -1))],
    [(0x3B, 3, [7, 6, 5]),        (0x38, 0, range(7, -1, -1))],
]

DOTS = [(0x3B, 0, 7), (0x38, 3, 0), (0x38, 1, 0), (0x3B, 1, 7)]
CHIPS = {"CHIP_38": 0x38, "CHIP_39": 0x39, "CHIP_3A": 0x3A, "CHIP_3B": 0x3B}


def build_map():
    """(chip, digit, bit) -> (zeile, spalte)"""
    m = {}
    for r, row in enumerate(ROWS):
        col = 0
        for chip, digit, bits in row:
            for b in bits:
                m[(chip, digit, b)] = (r, col)
                col += 1
        assert col == 11, f"Zeile {r + 1}: {col} statt 11 LEDs"
    return m


def parse_words(path):
    src = Path(path).read_text(encoding="utf-8")
    words = []
    for name, body in re.findall(r'\[W_\w+\]\s*=\s*\{\s*"([^"]+)",\s*\d+,\s*\{(.*?)\}\s*\}', src, re.S):
        parts = [(CHIPS[c], int(d), int(mk, 16))
                 for c, d, mk in re.findall(r"P\((CHIP_\w+),\s*(\d+),\s*(0x[0-9A-Fa-f]+)\)", body)]
        words.append((name, parts))
    return words


def check(words, m):
    errors = 0
    for name, parts in words:
        if name.startswith("•"):
            continue
        cells = []
        for chip, digit, mask in parts:
            for b in range(8):
                if mask & (1 << b):
                    if (chip, digit, b) not in m:
                        print(f"FEHLER {name}: 0x{chip:02x}[{digit}] Bit {b} liegt nicht im Raster")
                        errors += 1
                    else:
                        cells.append(m[(chip, digit, b)])
        cells.sort()
        text = "".join(GRID[r][c] for r, c in cells)
        rows = {r for r, _ in cells}
        contiguous = len(rows) == 1 and all(cells[i + 1][1] == cells[i][1] + 1 for i in range(len(cells) - 1))
        ok = text == name and contiguous
        print(f"{'ok    ' if ok else 'FEHLER'} {name:8s} -> {text}")
        errors += not ok
    return errors


def table(m):
    inv = {v: k for k, v in m.items()}
    out = ["| Zeile | " + " | ".join(str(c + 1) for c in range(11)) + " |",
           "|---|" + "---|" * 11]
    for r in range(10):
        cells = []
        for c in range(11):
            chip, digit, b = inv[(r, c)]
            cells.append(f"**{GRID[r][c]}**<br>`{chip:02x}[{digit}]`<br>`0x{1 << b:02X}`")
        out.append(f"| {r + 1} | " + " | ".join(cells) + " |")
    return "\n".join(out)


if __name__ == "__main__":
    m = build_map()
    if "--table" in sys.argv:
        print(table(m))
        sys.exit(0)
    words_c = Path(__file__).resolve().parent.parent / "c" / "words.c"
    n = check(parse_words(words_c), m)
    print("Alle Wörter passen zum Raster." if n == 0 else f"{n} Fehler.")
    sys.exit(1 if n else 0)
