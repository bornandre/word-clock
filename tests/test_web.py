#!/usr/bin/env python3
"""
Tests für wordclock-web im Demo-Modus (ohne Pi, nichts wird ausgeführt).
Aufruf: python3 tests/test_web.py   (auch Teil von "make check")
"""
import ast
import http.cookiejar
import json
import os
import re
import shutil
import socket
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.parse
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
WEB = os.path.join(HERE, "..", "web", "wordclock-web")

failures = 0


def check(name, cond, detail=""):
    global failures
    if cond:
        print("ok      %s" % name)
    else:
        failures += 1
        print("FEHLER  %s %s" % (name, detail))


class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, *args, **kwargs):
        return None


def main():
    # Syntax muss mit Python 3.9 (Raspberry Pi OS Bullseye) kompatibel sein
    with open(WEB, encoding="utf-8") as f:
        src = f.read()
    try:
        ast.parse(src, feature_version=(3, 9))
        check("Syntax Python 3.9", True)
    except SyntaxError as e:
        check("Syntax Python 3.9", False, str(e))

    root = tempfile.mkdtemp(prefix="wordclock-test-")
    os.makedirs(os.path.join(root, "etc/wordclock"))
    os.makedirs(os.path.join(root, "etc/wpa_supplicant"))
    with open(os.path.join(root, "etc/wordclock/wordclock.conf"), "w") as f:
        f.write("brightness = 3\n# Kommentar bleibt\n")
    with open(os.path.join(root, "etc/wpa_supplicant/wpa_supplicant.conf"), "w") as f:
        f.write('country=CH\n\nnetwork={\n\tssid="Zuhause"\n\tpsk="x12345678"\n}\n')

    s = socket.socket()
    s.bind(("127.0.0.1", 0))
    port = s.getsockname()[1]
    s.close()

    proc = subprocess.Popen([sys.executable, WEB, "--demo", "--root", root,
                             "--bind", "127.0.0.1", "--port", str(port)],
                            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    base = "http://127.0.0.1:%d" % port
    jar = http.cookiejar.CookieJar()
    opener = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(jar), NoRedirect)

    def req(path, data=None, headers=None):
        body = urllib.parse.urlencode(data).encode() if data is not None else None
        r = urllib.request.Request(base + path, data=body, headers=headers or {})
        try:
            resp = opener.open(r, timeout=10)
            return resp.status, resp.headers, resp.read().decode()
        except urllib.error.HTTPError as e:
            return e.code, e.headers, e.read().decode()

    def msg(path, data):
        code, h, _ = req(path, data)
        loc = h.get("Location", "")
        m = re.search(r"m=([a-z_]+)", loc)
        return m.group(1) if m else loc

    try:
        for _ in range(50):
            try:
                socket.create_connection(("127.0.0.1", port), timeout=0.2).close()
                break
            except OSError:
                time.sleep(0.1)

        code, _, body = req("/")
        check("Login-Seite ohne Anmeldung", code == 200 and "Anmelden" in body)
        check("Scan ohne Anmeldung gesperrt", req("/api/scan")[0] == 403)
        check("falsches Passwort", msg("/login", {"password": "falsch"}) == "login_failed")
        check("Anmeldung", req("/login", {"password": "wordclock"})[0] == 303)

        code, _, body = req("/")
        check("Hauptseite", code == 200 and "<h2>Anzeige</h2>" in body)
        csrf = re.search(r'name="csrf" value="([^"]+)"', body).group(1)

        scan = json.loads(req("/api/scan")[2])
        check("WLAN-Suche", [n["ssid"] for n in scan] == ["Zuhause", "Nachbar 5G", "Grüezi-Gast"])

        check("ohne CSRF-Token abgelehnt", msg("/display", {"brightness": "12"}) == "csrf")
        check("WLAN speichern", msg("/wifi", {"csrf": csrf, "ssid": "Grüezi-Gast",
                                              "psk": "passwort123"}) == "wifi_saved")
        check("WLAN-Passwort zu kurz", msg("/wifi", {"csrf": csrf, "ssid": "X",
                                                     "psk": "kurz"}) == "wifi_invalid")
        with open(os.path.join(root, "etc/wpa_supplicant/wpa_supplicant.conf")) as f:
            wpa = f.read()
        check("wpa_supplicant.conf: neues Netz", "ssid=4772c3bc657a692d47617374" in wpa and
              "psk=" in wpa and 'ssid="Zuhause"' in wpa)
        check("wpa_supplicant.conf: Rechte 600",
              oct(os.stat(os.path.join(root, "etc/wpa_supplicant/wpa_supplicant.conf")).st_mode
                  & 0o777) == "0o600")
        check("WLAN entfernen", msg("/wifi/remove", {"csrf": csrf, "ssid": "Zuhause"}) ==
              "wifi_removed")
        with open(os.path.join(root, "etc/wpa_supplicant/wpa_supplicant.conf")) as f:
            check("wpa_supplicant.conf: altes Netz entfernt", "Zuhause" not in f.read())

        check("Anzeige speichern", msg("/display", {
            "csrf": csrf, "brightness": "12", "night_enabled": "1", "night_start": "22:30",
            "night_end": "06:00", "night_brightness": "3"}) == "display_saved")
        check("Anzeige: ungültige Helligkeit", msg("/display", {
            "csrf": csrf, "brightness": "13", "night_start": "22:30", "night_end": "06:00",
            "night_brightness": "3"}) == "display_invalid")
        with open(os.path.join(root, "etc/wordclock/wordclock.conf")) as f:
            conf = f.read()
        check("wordclock.conf geschrieben, Kommentar erhalten",
              "brightness = 12" in conf and "night_start = 22:30" in conf and
              "# Kommentar bleibt" in conf)

        check("Zeit stellen", msg("/time", {"csrf": csrf, "datetime": "2026-11-06T15:30:00"})
              == "time_saved")
        check("ungültiges Datum", msg("/time", {"csrf": csrf, "datetime": "2026-02-30T10:00"})
              == "time_invalid")
        check("Zeitzone", msg("/timezone", {"csrf": csrf, "tz": "Europe/Zurich"}) == "tz_saved")
        check("unbekannte Zeitzone", msg("/timezone", {"csrf": csrf, "tz": "Mars/Base"})
              == "tz_invalid")

        check("Passwort: falsches aktuelles", msg("/password", {
            "csrf": csrf, "current": "x", "new": "abcdefgh", "repeat": "abcdefgh"}) == "pw_wrong")
        check("Passwort: zu kurz", msg("/password", {
            "csrf": csrf, "current": "wordclock", "new": "kurz", "repeat": "kurz"}) == "pw_invalid")
        check("Passwort ändern", msg("/password", {
            "csrf": csrf, "current": "wordclock", "new": "neu#Passwort",
            "repeat": "neu#Passwort"}) == "pw_saved")
        secret = os.path.join(root, "etc/wordclock/secret.conf")
        check("secret.conf: Rechte 600", oct(os.stat(secret).st_mode & 0o777) == "0o600")

        _, _, body = req("/?m=wifi_saved&a=" + urllib.parse.quote('["<script>"]'))
        check("Meldungen werden maskiert", "&lt;script&gt;" in body and "<script>\"" not in body)

        with open(os.path.join(root, "run/wordclock/state"), "w") as f:
            f.write("ap\n")
        code, h, _ = req("/generate_204", headers={"Host": "connectivitycheck.gstatic.com"})
        check("Captive Portal leitet um", code == 303 and h.get("Location") == "http://10.42.0.1/")
        check("Hotspot-Adresse direkt erreichbar", req("/", headers={"Host": "10.42.0.1"})[0] == 200)

        check("Abmelden", msg("/logout", {"csrf": csrf}) == "logged_out")
        check("nach Abmelden gesperrt", "Anmelden" in req("/")[2])
    finally:
        proc.terminate()
        proc.wait(timeout=5)
        shutil.rmtree(root, ignore_errors=True)

    print("Webseite: alle Tests bestanden." if failures == 0 else "%d Fehler." % failures)
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
