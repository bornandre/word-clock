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

HDR      = i2c.h saa1064.h words.h config.h
COMMON   = i2c.c saa1064.c words.c config.c

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
#   piclock       -> /usr/local/bin/piclock (+ piclock-test-leds)
#   Webseite      -> /usr/local/sbin/wordclock-web
#   Einstellungen -> /etc/wordclock/ (nur wenn noch nicht vorhanden)
#   Dienste       -> piclock.service, wordclock-web.service
#
# WLAN-Hotspot:   sudo make install-wifi   (separat, ändert /etc/dhcpcd.conf)
# ---------------------------------------------------------------------------
PREFIX     ?= /usr/local
BINDIR      = $(DESTDIR)$(PREFIX)/bin
SBINDIR     = $(DESTDIR)$(PREFIX)/sbin
SYSTEMDDIR ?= /etc/systemd/system
UNITDIR     = $(DESTDIR)$(SYSTEMDDIR)
CONFDIR     = $(DESTDIR)/etc/wordclock
DHCPCD_CONF = $(DESTDIR)/etc/dhcpcd.conf

# systemctl nur beim echten Installieren aufrufen (nicht bei DESTDIR-Paketbau)
ifeq ($(DESTDIR),)
    SYSTEMCTL := systemctl
else
    SYSTEMCTL := @true
endif

install: $(PICLOCK_TARGET) test_leds
	install -d $(BINDIR) $(SBINDIR) $(UNITDIR) $(CONFDIR)
	install -m 755 $(PICLOCK_TARGET)          $(BINDIR)/piclock
	install -m 755 test_leds                  $(BINDIR)/piclock-test-leds
	install -m 755 ../web/wordclock-web       $(SBINDIR)/wordclock-web
	install -m 644 ../systemd/piclock.service       $(UNITDIR)/
	install -m 644 ../systemd/wordclock-web.service $(UNITDIR)/
	@[ -e $(CONFDIR)/wordclock.conf ] || install -m 644 ../config/wordclock.conf $(CONFDIR)/
	@[ -e $(CONFDIR)/secret.conf ]    || install -m 600 ../config/secret.conf    $(CONFDIR)/
	$(SYSTEMCTL) daemon-reload
	$(SYSTEMCTL) enable piclock.service wordclock-web.service
	$(SYSTEMCTL) restart piclock.service wordclock-web.service
	@echo
	@echo "Installiert. Webseite: http://$$(hostname).local  (Passwort: wordclock)"
	@echo "Für den Setup-Hotspot zusätzlich: sudo make install-wifi"

install-wifi:
	@command -v hostapd >/dev/null && command -v dnsmasq >/dev/null && command -v iw >/dev/null || \
	    { echo "Bitte zuerst: sudo apt install hostapd dnsmasq-base iw"; exit 1; }
	@if systemctl is-active -q dnsmasq 2>/dev/null; then \
	    echo "Der Dienst dnsmasq läuft bereits und würde den Hotspot stören."; \
	    echo "Falls nicht anderweitig gebraucht: sudo systemctl disable --now dnsmasq"; exit 1; fi
	@if [ -z "$(DESTDIR)" ] && ! systemctl is-active -q dhcpcd; then \
	    echo "wordclock-wifi ist für dhcpcd (Raspberry Pi OS Bullseye) gebaut, dhcpcd läuft nicht."; \
	    exit 1; fi
	install -d $(SBINDIR) $(UNITDIR) $(CONFDIR)
	install -m 755 ../wifi/wordclock-wifi $(SBINDIR)/wordclock-wifi
	install -m 644 ../systemd/wordclock-wifi.service $(UNITDIR)/
	@[ -e $(CONFDIR)/wordclock.conf ] || install -m 644 ../config/wordclock.conf $(CONFDIR)/
	@[ -e $(CONFDIR)/secret.conf ]    || install -m 600 ../config/secret.conf    $(CONFDIR)/
	-$(SYSTEMCTL) disable --now hostapd.service 2>/dev/null
	-$(SYSTEMCTL) mask hostapd.service
	@touch $(DHCPCD_CONF)
	@if ! grep -q '^nohook wpa_supplicant' $(DHCPCD_CONF); then \
	    sed -i '1i # wordclock-wifi: wpa_supplicant wird von wordclock-wifi gestartet\nnohook wpa_supplicant\n' $(DHCPCD_CONF); \
	    echo "/etc/dhcpcd.conf: 'nohook wpa_supplicant' eingetragen"; fi
	$(SYSTEMCTL) daemon-reload
	$(SYSTEMCTL) enable wordclock-wifi.service
	$(SYSTEMCTL) restart wordclock-wifi.service
	@echo
	@echo "WLAN-Hotspot installiert. Protokoll: journalctl -u wordclock-wifi -f"

uninstall:
	-$(SYSTEMCTL) disable --now piclock.service wordclock-web.service
	rm -f $(BINDIR)/piclock $(BINDIR)/piclock-test-leds $(SBINDIR)/wordclock-web
	rm -f $(UNITDIR)/piclock.service $(UNITDIR)/wordclock-web.service
	$(SYSTEMCTL) daemon-reload
	@echo "Einstellungen in /etc/wordclock wurden behalten."

uninstall-wifi:
	-$(SYSTEMCTL) disable --now wordclock-wifi.service
	rm -f $(SBINDIR)/wordclock-wifi $(UNITDIR)/wordclock-wifi.service
	-sed -i '/^# wordclock-wifi: wpa_supplicant wird/d; /^nohook wpa_supplicant$$/d' $(DHCPCD_CONF)
	-$(SYSTEMCTL) unmask hostapd.service
	$(SYSTEMCTL) daemon-reload
	@echo "Bitte neu starten (sudo reboot), damit dhcpcd das WLAN wieder selbst verwaltet."

# ---------------------------------------------------------------------------
# Tests (ohne Hardware):  make check
#   1. Wort-Tabelle auf überlappende LEDs prüfen
#   2. Alle 1440 Minuten eines Tages im Trockenlauf mit der Referenz vergleichen
#   3. Einstellungsdatei lesen (gültig und fehlerhaft)
#   4. WLAN-Skript (Syntax) und Webseite (Demo-Modus)
# ---------------------------------------------------------------------------
check: $(PICLOCK_TARGET) test_leds
	./test_leds -c
	./$(PICLOCK_TARGET) -n -a > ../tests/actual_times.txt
	diff -u ../tests/expected_times.txt ../tests/actual_times.txt
	@rm -f ../tests/actual_times.txt
	cd ../tests/config && ../../c/$(PICLOCK_TARGET) -p -c valid.conf   2>&1 | diff -u valid.expected -
	cd ../tests/config && ../../c/$(PICLOCK_TARGET) -p -c invalid.conf 2>&1 | diff -u invalid.expected -
	bash -n ../wifi/wordclock-wifi
	python3 ../tests/test_web.py
	@echo "Alle Tests bestanden."

clean:
	rm -f *.o piclock piclock32 test_leds ../tests/actual_times.txt

.PHONY: all clean install uninstall install-wifi uninstall-wifi check
