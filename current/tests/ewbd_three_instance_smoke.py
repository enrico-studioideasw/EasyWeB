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


with tempfile.TemporaryDirectory(prefix="ewbd-three-instance-") as temporary:
    base = pathlib.Path(temporary)
    instances = []

    for number in range(1, 4):
        root = base / f"node{number}"
        root.mkdir()
        content = f"node {number}\n".encode()
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
        instances.append((server, port, content))

    try:
        for server, port, content in instances:
            for _ in range(50):
                try:
                    get_headers, get_body = request(port, "GET")
                    break
                except OSError:
                    if server.poll() is not None:
                        raise RuntimeError(server.stderr.read())
                    time.sleep(0.05)
            else:
                raise RuntimeError(f"ewbd on port {port} did not start")

            head_headers, head_body = request(port, "HEAD")
            expected_length = f"Content-Length: {len(content)}".encode()
            if get_body != content or expected_length not in get_headers:
                raise AssertionError(f"GET failed or used the wrong root on port {port}")
            if expected_length not in head_headers or head_body:
                raise AssertionError(f"HEAD contract failed on port {port}")
    finally:
        for server, _, _ in instances:
            if server.poll() is None:
                server.send_signal(signal.SIGTERM)
        for server, _, _ in instances:
            try:
                server.wait(timeout=3)
            except subprocess.TimeoutExpired:
                server.kill()
                server.wait()
