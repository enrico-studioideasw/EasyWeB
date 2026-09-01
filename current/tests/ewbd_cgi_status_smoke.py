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


def request(port, query):
    payload = f"GET /probe.cgi?{query} HTTP/1.0\r\nHost: localhost\r\n\r\n"
    with socket.create_connection(("127.0.0.1", port), timeout=2) as client:
        client.sendall(payload.encode())
        response = b""
        while True:
            chunk = client.recv(4096)
            if not chunk:
                break
            response += chunk
    headers, separator, body = response.partition(b"\r\n\r\n")
    if not separator:
        raise AssertionError(f"missing header separator: {response!r}")
    return headers.split(b"\r\n"), body


with tempfile.TemporaryDirectory(prefix="ewbd-cgi-status-") as temporary:
    root = pathlib.Path(temporary)
    probe = root / "probe.cgi"
    probe.write_text(
        """#!/usr/bin/python3
import os

query = os.environ.get('QUERY_STRING', '')
if query == 'unauthorized':
    print('Status: 401 Unauthorized')
elif query == 'redirect':
    print('Status: 303 See Other')
    print('Location: ./')
print('Content-Type: text/plain')
print()
print(query or 'default')
"""
    )
    probe.chmod(0o755)

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
            str(root / "stats.log"),
        ],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        text=True,
    )

    try:
        for _ in range(50):
            try:
                default_headers, default_body = request(port, "")
                break
            except OSError:
                if server.poll() is not None:
                    raise RuntimeError(server.stderr.read())
                time.sleep(0.05)
        else:
            raise RuntimeError("ewbd did not start")

        cases = [
            (default_headers, default_body, b"HTTP/1.0 200 OK", b"default\n"),
            (*request(port, "unauthorized"), b"HTTP/1.0 401 Unauthorized", b"unauthorized\n"),
            (*request(port, "redirect"), b"HTTP/1.0 303 See Other", b"redirect\n"),
        ]
        for headers, body, expected_status, expected_body in cases:
            if headers[0] != expected_status:
                raise AssertionError(f"status mismatch: {headers!r}")
            if any(header.lower().startswith(b"status:") for header in headers[1:]):
                raise AssertionError(f"CGI Status leaked as response header: {headers!r}")
            if body != expected_body:
                raise AssertionError(f"body mismatch: {body!r}")

        redirect_headers = request(port, "redirect")[0]
        if b"Location: ./" not in redirect_headers:
            raise AssertionError(f"redirect location missing: {redirect_headers!r}")
    finally:
        server.send_signal(signal.SIGTERM)
        try:
            server.wait(timeout=3)
        except subprocess.TimeoutExpired:
            server.kill()
            server.wait()

