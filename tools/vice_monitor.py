"""Local reference-emulator monitor. The original disk is never modified."""
from __future__ import annotations

import argparse
import socket
import time


class Monitor:
    def __init__(self, port: int = 6510):
        self.socket = socket.create_connection(("127.0.0.1", port), timeout=5)
        self.socket.settimeout(1)
        self.read()

    def read(self, timeout: float = 2) -> str:
        data = bytearray()
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            try:
                part = self.socket.recv(65536)
                if not part:
                    break
                data.extend(part)
                if data.rstrip().endswith(b")") or data.rstrip().endswith(b"."):
                    break
            except socket.timeout:
                break
        return data.decode("latin1")

    def command(self, command: str, timeout: float = 3) -> str:
        self.socket.sendall((command + "\n").encode("ascii"))
        return self.read(timeout)

    def close(self):
        self.socket.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("commands", nargs="+")
    args = parser.parse_args()
    monitor = Monitor()
    for command in args.commands:
        print(monitor.command(command))
    monitor.close()
