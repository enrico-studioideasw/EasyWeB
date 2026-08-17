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


def request(port, target):
    with socket.create_connection(("127.0.0.1", port), timeout=2) as client:
        client.sendall(
            f"GET {target} HTTP/1.0\r\nHost: localhost\r\n\r\n".encode()
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


with tempfile.TemporaryDirectory(prefix="ewbd-evm-resolver-") as temporary:
    root = pathlib.Path(temporary)
    source = root / "ft.ewb"
    source.write_text('print("resolver");\n', encoding="utf-8")
    subprocess.run(
        ["./build/ewb", str(source), str(root / "ft.evm")],
        check=True,
        stdout=subprocess.DEVNULL,
    )
    (root / "plain.txt").write_text("plain\n", encoding="utf-8")

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
                implicit_headers, implicit_body = request(port, "/ft?value=42")
                break
            except OSError:
                if server.poll() is not None:
                    raise RuntimeError(server.stderr.read())
                time.sleep(0.05)
        else:
            raise RuntimeError("ewbd did not start")

        explicit_headers, explicit_body = request(port, "/ft.evm?value=42")
        missing_headers, _ = request(port, "/missing")
        plain_headers, plain_body = request(port, "/plain.txt")

        if b"200 OK" not in implicit_headers or implicit_body != explicit_body:
            raise AssertionError("implicit and explicit EVM paths differ")
        if b"404 Not Found" not in missing_headers:
            raise AssertionError("missing extensionless path did not remain missing")
        if b"200 OK" not in plain_headers or plain_body != b"plain\n":
            raise AssertionError("path with an extension changed behavior")
    finally:
        server.send_signal(signal.SIGTERM)
        try:
            server.wait(timeout=3)
        except subprocess.TimeoutExpired:
            server.kill()
            server.wait()
