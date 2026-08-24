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


def request(port, headers):
    payload = "GET /probe.cgi HTTP/1.0\r\nHost: localhost\r\n"
    payload += "".join(f"{name}: {value}\r\n" for name, value in headers)
    payload += "\r\n"
    with socket.create_connection(("127.0.0.1", port), timeout=2) as client:
        client.sendall(payload.encode())
        response = b""
        while True:
            chunk = client.recv(4096)
            if not chunk:
                break
            response += chunk
    response_headers, separator, body = response.partition(b"\r\n\r\n")
    if not separator or b" 200 " not in response_headers:
        raise AssertionError(f"unexpected response: {response!r}")
    return body


with tempfile.TemporaryDirectory(prefix="ewbd-cgi-headers-") as temporary:
    root = pathlib.Path(temporary)
    probe = root / "probe.cgi"
    probe.write_text(
        """#!/usr/bin/python3
import os

print('Content-Type: text/plain')
print()
print('cookie=' + os.environ.get('HTTP_COOKIE', '<missing>'))
print('authorization=' + os.environ.get('HTTP_AUTHORIZATION', '<missing>'))
print('unrelated=' + os.environ.get('HTTP_X_EWBD_PROBE', '<missing>'))
"""
    )
    probe.chmod(0o755)

    port = free_port()
    environment = {
        "PATH": "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin",
        "HTTP_COOKIE": "inherited-cookie",
        "HTTP_AUTHORIZATION": "inherited-authorization",
        "HTTP_X_EWBD_PROBE": "inherited-unrelated",
    }
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
        env=environment,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        text=True,
    )

    try:
        for _ in range(50):
            try:
                first = request(
                    port,
                    [
                        ("Cookie", "forum_token=opaque"),
                        ("Authorization", "Bearer opaque"),
                        ("X-Ewbd-Probe", "must-not-be-exported"),
                    ],
                )
                break
            except OSError:
                if server.poll() is not None:
                    raise RuntimeError(server.stderr.read())
                time.sleep(0.05)
        else:
            raise RuntimeError("ewbd did not start")

        expected = (
            b"cookie=forum_token=opaque\n"
            b"authorization=Bearer opaque\n"
            b"unrelated=inherited-unrelated\n"
        )
        if first != expected:
            raise AssertionError(f"selected CGI headers changed: {first!r}")

        second = request(port, [])
        expected_absent = (
            b"cookie=\n"
            b"authorization=\n"
            b"unrelated=inherited-unrelated\n"
        )
        if second != expected_absent:
            raise AssertionError(f"CGI request headers leaked: {second!r}")
    finally:
        server.send_signal(signal.SIGTERM)
        try:
            server.wait(timeout=3)
        except subprocess.TimeoutExpired:
            server.kill()
            server.wait()
