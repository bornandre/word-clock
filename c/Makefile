CC     = gcc
CFLAGS = -Wall -Wextra -O2 -std=gnu11 $(CFLAGS_EXTRA)

ARCH := $(shell uname -m)

ifeq ($(ARCH),armv7l)
    PICLOCK_TARGET := piclock32
    ARCHFLAGS      := -marm
else
    PICLOCK_TARGET := piclock
    ARCHFLAGS      :=
endif

HDR      = i2c.h saa1064.h words.h
COMMON   = i2c.c saa1064.c words.c

SRC      = main.c $(COMMON)
OBJ      = $(SRC:.c=.o)

TEST_SRC = test_leds.c $(COMMON)
TEST_OBJ = $(TEST_SRC:.c=.o)

all: $(PICLOCK_TARGET) test_leds

$(PICLOCK_TARGET): $(OBJ)
	$(CC) $(ARCHFLAGS) $(OBJ) -o $@

test_leds: $(TEST_OBJ)
	$(CC) $(ARCHFLAGS) $(TEST_OBJ) -o $@

%.o: %.c $(HDR)
	$(CC) $(CFLAGS) $(ARCHFLAGS) -c $< -o $@

# ---------------------------------------------------------------------------
# Installation:   sudo make install
#   Programm  -> /usr/local/bin/piclock   (auch auf 32 Bit unter diesem Namen)
#   Service   -> /etc/systemd/system/piclock.service
# ---------------------------------------------------------------------------
PREFIX     ?= /usr/local
BINDIR      = $(DESTDIR)$(PREFIX)/bin
SYSTEMDDIR ?= /etc/systemd/system
UNITDIR     = $(DESTDIR)$(SYSTEMDDIR)
SERVICE     = piclock.service
SERVICE_SRC = ../systemd/$(SERVICE)

# systemctl nur beim echten Installieren aufrufen (nicht bei DESTDIR-Paketbau)
ifeq ($(DESTDIR),)
    SYSTEMCTL := systemctl
else
    SYSTEMCTL := @true
endif

install: $(PICLOCK_TARGET) test_leds
	install -d $(BINDIR) $(UNITDIR)
	install -m 755 $(PICLOCK_TARGET) $(BINDIR)/piclock
	install -m 755 test_leds         $(BINDIR)/piclock-test-leds
	install -m 644 $(SERVICE_SRC)    $(UNITDIR)/$(SERVICE)
	$(SYSTEMCTL) daemon-reload
	$(SYSTEMCTL) enable $(SERVICE)
	$(SYSTEMCTL) restart $(SERVICE)
	@echo "Installiert. Status: systemctl status piclock"

uninstall:
	-$(SYSTEMCTL) disable --now $(SERVICE)
	rm -f $(BINDIR)/piclock $(BINDIR)/piclock-test-leds $(UNITDIR)/$(SERVICE)
	$(SYSTEMCTL) daemon-reload

# ---------------------------------------------------------------------------
# Tests (ohne Hardware):  make check
#   1. Wort-Tabelle auf überlappende LEDs prüfen
#   2. Alle 1440 Minuten eines Tages im Trockenlauf mit der Referenz vergleichen
# ---------------------------------------------------------------------------
check: $(PICLOCK_TARGET) test_leds
	./test_leds -c
	./$(PICLOCK_TARGET) -n -a > ../tests/actual_times.txt
	diff -u ../tests/expected_times.txt ../tests/actual_times.txt
	@rm -f ../tests/actual_times.txt
	@echo "Alle Tests bestanden."

clean:
	rm -f *.o piclock piclock32 test_leds ../tests/actual_times.txt

.PHONY: all clean install uninstall check
