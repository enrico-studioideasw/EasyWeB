#!/usr/bin/env python3

import pathlib
import signal
import socket
import subprocess
import tempfile
import time


def free_port():
    sock = socket.socket()
    sock.bind(("127.0.0.1", 0))
    port = sock.getsockname()[1]
    sock.close()
    return port


def request(port, method):
    with socket.create_connection(("127.0.0.1", port), timeout=2) as client:
        client.sendall(
            f"{method} /index.html HTTP/1.0\r\nHost: localhost\r\n\r\n".encode()
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


with tempfile.TemporaryDirectory(prefix="ewbd-static-http-") as temporary:
    root = pathlib.Path(temporary)
    content = b"static smoke body\n"
    (root / "index.html").write_bytes(content)
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
                get_headers, get_body = request(port, "GET")
                break
            except OSError:
                if server.poll() is not None:
                    raise RuntimeError(server.stderr.read())
                time.sleep(0.05)
        else:
            raise RuntimeError("ewbd did not start")

        head_headers, head_body = request(port, "HEAD")
        expected_length = f"Content-Length: {len(content)}".encode()
        if get_body != content or expected_length not in get_headers:
            raise AssertionError("GET static response changed")
        if expected_length not in head_headers:
            raise AssertionError("HEAD did not preserve GET Content-Length")
        if head_body:
            raise AssertionError(f"HEAD returned {len(head_body)} body bytes")
    finally:
        server.send_signal(signal.SIGTERM)
        try:
            server.wait(timeout=3)
        except subprocess.TimeoutExpired:
            server.kill()
            server.wait()
