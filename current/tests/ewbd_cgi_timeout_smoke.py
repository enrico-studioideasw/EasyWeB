#!/usr/bin/env python3

import pathlib
import signal
import socket
import subprocess
import tempfile
import time


def free_port():
    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        return sock.getsockname()[1]


def request(port, path, timeout=5):
    with socket.create_connection(("127.0.0.1", port), timeout=timeout) as client:
        client.sendall(f"GET {path} HTTP/1.0\r\nHost: localhost\r\n\r\n".encode())
        response = b""
        while True:
            chunk = client.recv(4096)
            if not chunk:
                break
            response += chunk
    return response


def run_case(root, cgi_timeout, expected):
    port = free_port()
    server = subprocess.Popen(
        [
            "./build/ewbd",
            "-p",
            str(port),
            "-m",
            "1",
            "-M",
            "1",
            "--www",
            str(root),
            "--stats-file",
            str(root / f"stats-{cgi_timeout}.log"),
            "--cgi-timeout",
            str(cgi_timeout),
        ],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        text=True,
    )
    try:
        for _ in range(50):
            try:
                response = request(port, "/slow.cgi")
                break
            except OSError:
                if server.poll() is not None:
                    raise RuntimeError(server.stderr.read())
                time.sleep(0.05)
        else:
            raise RuntimeError("ewbd did not start")
        if expected not in response:
            raise AssertionError(f"unexpected CGI response: {response!r}")
    finally:
        if server.poll() is None:
            server.send_signal(signal.SIGTERM)
        try:
            server.wait(timeout=3)
        except subprocess.TimeoutExpired:
            server.kill()
            server.wait()


with tempfile.TemporaryDirectory(prefix="ewbd-cgi-timeout-") as temporary:
    root = pathlib.Path(temporary)
    slow_cgi = root / "slow.cgi"
    slow_cgi.write_text(
        "#!/bin/sh\n"
        "sleep 2\n"
        "printf 'Content-Type: text/plain\\r\\n\\r\\nslow CGI complete\\n'\n"
    )
    slow_cgi.chmod(0o755)

    run_case(root, 1, b"504 Gateway Timeout")
    run_case(root, 3, b"slow CGI complete")
