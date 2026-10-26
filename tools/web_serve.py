#!/usr/bin/env python3
"""Локальный сервер для браузерной сборки (W1): отдаёт папку с заголовками COOP и COEP — без них у страницы нет"""

import argparse
import functools
import http.client
import http.server
import socket
import ssl
import threading
import urllib.parse
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

PROXY_PREFIX = "/game-api"
HOP = {"host", "connection", "keep-alive", "transfer-encoding", "upgrade", "proxy-connection", "te", "trailer",
       "content-length", "accept-encoding", "origin", "referer", "server", "date"}

class Handler(http.server.SimpleHTTPRequestHandler):
    ipa = None
    proxy = None
    extensions_map = {**http.server.SimpleHTTPRequestHandler.extensions_map,
                      ".wasm": "application/wasm", ".js": "text/javascript", ".mjs": "text/javascript"}

    def translate_path(self, path):
        if self.ipa and path.split("?", 1)[0] == "/game.ipa":
            return self.ipa
        return super().translate_path(path)

    def proxied(self):
        return self.proxy and (self.path == PROXY_PREFIX or self.path.startswith(PROXY_PREFIX + "/"))

    def do_GET(self):
        if not self.proxied():
            return super().do_GET()
        if self.headers.get("Upgrade", "").lower() == "websocket":
            return self.tunnel()
        self.forward()

    def do_POST(self):
        self.forward() if self.proxied() else self.send_error(405)

    def do_PUT(self):
        self.forward() if self.proxied() else self.send_error(405)

    def do_DELETE(self):
        self.forward() if self.proxied() else self.send_error(405)

    def do_PATCH(self):
        self.forward() if self.proxied() else self.send_error(405)

    def upstream(self):
        scheme, host, port = self.proxy
        if scheme == "https":
            return http.client.HTTPSConnection(host, port, timeout=60)
        return http.client.HTTPConnection(host, port, timeout=60)

    def forward(self):
        path = self.path[len(PROXY_PREFIX):] or "/"
        length = int(self.headers.get("Content-Length") or 0)
        body = self.rfile.read(length) if length else None
        headers = {k: v for k, v in self.headers.items() if k.lower() not in HOP}
        conn = self.upstream()
        try:
            conn.request(self.command, path, body=body, headers=headers)
            r = conn.getresponse()
            data = r.read()
        except OSError as e:
            self.send_error(502, str(e))
            return
        finally:
            conn.close()
        self.send_response(r.status, r.reason)
        for k, v in r.getheaders():
            if k.lower() not in HOP and k.lower() != "content-encoding" and not k.lower().startswith("access-control-"):
                self.send_header(k, v)
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def tunnel(self):
        scheme, host, port = self.proxy
        raw = socket.create_connection((host, port), timeout=30)
        up = ssl.create_default_context().wrap_socket(raw, server_hostname=host) if scheme == "https" else raw
        path = self.path[len(PROXY_PREFIX):] or "/"
        lines = [f"GET {path} HTTP/1.1", f"Host: {host}"]
        lines += [f"{k}: {v}" for k, v in self.headers.items() if k.lower() not in ("host", "origin", "referer")]
        up.sendall(("\r\n".join(lines) + "\r\n\r\n").encode())
        up.settimeout(None)
        client = self.connection
        client.settimeout(None)

        def pipe(src, dst):
            try:
                while True:
                    chunk = src.recv(65536)
                    if not chunk:
                        break
                    dst.sendall(chunk)
            except OSError:
                pass
            for s in (src, dst):
                try:
                    s.shutdown(socket.SHUT_RDWR)
                except OSError:
                    pass

        t = threading.Thread(target=pipe, args=(up, client), daemon=True)
        t.start()
        pipe(client, up)
        t.join()
        self.close_connection = True

    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("Cross-Origin-Resource-Policy", "same-origin")
        self.send_header("Cache-Control", "no-cache")
        super().end_headers()

def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--dir", default=str(ROOT / "build" / "web" / "game"))
    ap.add_argument("--port", type=int, default=8090)
    ap.add_argument("--ipa", help="своя копия игры — отдавать как /game.ipa")
    ap.add_argument("--proxy-game", nargs="?", const="https://game.mooncellar.space", default=None,
                    help="пути /game-api/… — на сервер игры (по умолчанию https://game.mooncellar.space)")
    a = ap.parse_args()
    Handler.ipa = str(Path(a.ipa).resolve()) if a.ipa else None
    if a.proxy_game:
        u = urllib.parse.urlparse(a.proxy_game)
        Handler.proxy = (u.scheme, u.hostname, u.port or (443 if u.scheme == "https" else 80))
    handler = functools.partial(Handler, directory=a.dir)
    page = "" if (Path(a.dir) / "index.html").exists() else "medecomp.html"
    with http.server.ThreadingHTTPServer(("127.0.0.1", a.port), handler) as server:
        print(f"http://localhost:{a.port}/{page} — {a.dir}", flush=True)
        server.serve_forever()

if __name__ == "__main__":
    main()
