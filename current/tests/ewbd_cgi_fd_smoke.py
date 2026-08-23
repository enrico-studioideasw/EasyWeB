#!/usr/bin/env python3

import os
import pathlib
import signal
import socket
import subprocess
import tempfile
import time
import urllib.request


def free_port():
    sock = socket.socket()
    sock.bind(("127.0.0.1", 0))
    port = sock.getsockname()[1]
    sock.close()
    return port


with tempfile.TemporaryDirectory(prefix="ewbd-cgi-fd-") as temporary:
    root = pathlib.Path(temporary)
    result = root / "inherited-fds.txt"
    stats = root / "stats.log"
    probe = root / "probe.cgi"
    probe.write_text(
        """#!/usr/bin/python3
import os
import time

if os.environ.get('SCRIPT_NAME') != '/probe.cgi':
    raise RuntimeError('unexpected SCRIPT_NAME')
if os.environ.get('SCRIPT_FILENAME') != os.path.realpath(__file__):
    raise RuntimeError('unexpected SCRIPT_FILENAME')
if os.environ.get('QUERY_STRING') != 'contract=full':
    raise RuntimeError('unexpected QUERY_STRING')

if os.fork() == 0:
    inherited = []
    for name in os.listdir('/proc/self/fd'):
        try:
            descriptor = int(name)
            if descriptor > 2:
                inherited.append((descriptor, os.readlink('/proc/self/fd/' + name)))
        except (FileNotFoundError, ValueError):
            pass
    os.close(0)
    os.close(1)
    os.close(2)
    with open(os.environ['EWBD_FD_RESULT'], 'w') as output:
        for descriptor, target in inherited:
            output.write(f'{descriptor} {target}\\n')
    time.sleep(0.2)
    os._exit(0)

print('Content-Type: text/plain')
print()
print('ok')
"""
    )
    probe.chmod(0o755)

    port = free_port()
    environment = os.environ.copy()
    environment["EWBD_FD_RESULT"] = str(result)
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
            str(stats),
        ],
        env=environment,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        text=True,
    )

    try:
        response = None
        for _ in range(50):
            try:
                response = urllib.request.urlopen(
                    f"http://127.0.0.1:{port}/probe.cgi?contract=full", timeout=2
                ).read()
                break
            except Exception:
                if server.poll() is not None:
                    raise RuntimeError(server.stderr.read())
                time.sleep(0.05)
        if response != b"ok\n":
            raise AssertionError(f"unexpected CGI response: {response!r}")

        for _ in range(50):
            if result.exists():
                break
            time.sleep(0.05)
        if not result.exists():
            raise AssertionError("CGI descendant did not report its descriptors")
        inherited = result.read_text().strip()
        if inherited:
            raise AssertionError(f"CGI descendant inherited descriptors:\n{inherited}")
    finally:
        server.send_signal(signal.SIGTERM)
        try:
            server.wait(timeout=3)
        except subprocess.TimeoutExpired:
            server.kill()
            server.wait()
