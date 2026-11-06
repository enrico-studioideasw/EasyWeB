#!/usr/bin/env python3

import os
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


def request(port):
    with socket.create_connection(("127.0.0.1", port), timeout=2) as client:
        client.sendall(
            b"GET /request?topic=2&toc HTTP/1.0\r\n"
            b"Host: localhost\r\n"
            b"Cookie: forum_token=cookie-value\r\n"
            b"Authorization: Bearer token-value\r\n\r\n"
        )
        response = b""
        while True:
            chunk = client.recv(4096)
            if not chunk:
                break
            response += chunk
    headers, separator, body = response.partition(b"\r\n\r\n")
    if not separator:
        raise AssertionError(f"malformed response: {response!r}")
    return headers, body


with tempfile.TemporaryDirectory(prefix="ewbd-request-") as temporary:
    root = pathlib.Path(temporary)
    subprocess.run(
        ["./build/ewb", "tests/request_smoke.ewb", str(root / "request.evm")],
        check=True,
        stdout=subprocess.DEVNULL,
    )
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
                headers, body = request(port)
                break
            except OSError:
                if server.poll() is not None:
                    raise RuntimeError(server.stderr.read())
                time.sleep(0.05)
        else:
            raise RuntimeError("ewbd did not start")

        if b"200 OK" not in headers:
            raise AssertionError(headers)
        expected = (
            b"GET|/request|topic=2&toc|forum_token=cookie-value|"
            b"Bearer token-value"
        )
        if body != expected:
            raise AssertionError((body, expected))

        cgi_env = os.environ.copy()
        cgi_env.update(
            {
                "REQUEST_METHOD": "GET",
                "SCRIPT_NAME": "/request",
                "QUERY_STRING": "topic=2&toc",
                "HTTP_COOKIE": "forum_token=cookie-value",
                "HTTP_AUTHORIZATION": "Bearer token-value",
            }
        )
        cgi = subprocess.run(
            ["./build/ewb_cgi", str(root / "request.evm")],
            check=True,
            capture_output=True,
            env=cgi_env,
        ).stdout
        if not cgi.endswith(b"\r\n\r\n" + expected):
            raise AssertionError(cgi)
    finally:
        server.send_signal(signal.SIGTERM)
        try:
            server.wait(timeout=3)
        except subprocess.TimeoutExpired:
            server.kill()
            server.wait()
