#!/usr/bin/env python3

import pathlib
import signal
import socket
import subprocess
import tempfile
import threading
import time


def free_port():
    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        return sock.getsockname()[1]


def request(port, path, timeout=15):
    with socket.create_connection(("127.0.0.1", port), timeout=timeout) as client:
        client.sendall(f"GET {path} HTTP/1.0\r\nHost: localhost\r\n\r\n".encode())
        response = b""
        while True:
            chunk = client.recv(4096)
            if not chunk:
                break
            response += chunk
    return response


with tempfile.TemporaryDirectory(prefix="ewbd-cgi-pool-") as temporary:
    root = pathlib.Path(temporary)
    (root / "index.html").write_text("static responder is available\n")
    cgi_started = root / "cgi-started"
    slow_cgi = root / "slow.cgi"
    slow_cgi.write_text(
        "#!/bin/sh\n"
        f": > '{cgi_started}'\n"
        "sleep 2\n"
        "printf 'Content-Type: text/plain\\r\\n\\r\\nslow CGI complete\\n'\n"
    )
    slow_cgi.chmod(0o755)

    port = free_port()
    server = subprocess.Popen(
        [
            "./build/ewbd",
            "-p",
            str(port),
            "-m",
            "1",
            "-M",
            "2",
            "--www",
            str(root),
            "--stats-file",
            str(root / "stats.log"),
        ],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        text=True,
    )

    try:
        for _ in range(50):
            try:
                request(port, "/index.html")
                break
            except OSError:
                if server.poll() is not None:
                    raise RuntimeError(server.stderr.read())
                time.sleep(0.05)
        else:
            raise RuntimeError("ewbd did not start")
        time.sleep(0.1)

        slow_result = []
        slow_thread = threading.Thread(
            target=lambda: slow_result.append(request(port, "/slow.cgi"))
        )
        slow_thread.start()
        for _ in range(50):
            if cgi_started.exists():
                break
            if not slow_thread.is_alive():
                raise AssertionError("slow CGI stopped before its start marker")
            time.sleep(0.02)
        else:
            raise AssertionError("slow CGI did not write its start marker")

        started = time.monotonic()
        static_response = request(port, "/index.html")
        static_elapsed = time.monotonic() - started

        slow_thread.join(timeout=4)
        if slow_thread.is_alive():
            raise AssertionError("slow CGI did not complete")
        if b"slow CGI complete" not in slow_result[0]:
            raise AssertionError(f"unexpected CGI response: {slow_result[0]!r}")
        if b"static responder is available" not in static_response:
            raise AssertionError(f"unexpected static response: {static_response!r}")
        if static_elapsed >= 1.5:
            raise AssertionError(
                f"static request waited {static_elapsed:.3f}s for the slow CGI"
            )
    finally:
        if server.poll() is None:
            server.send_signal(signal.SIGTERM)
        try:
            server.wait(timeout=3)
        except subprocess.TimeoutExpired:
            server.kill()
            server.wait()
