"""Small synchronous client for VICE's documented binary monitor protocol."""
import socket
import struct
from pathlib import Path


class Vice:
    def __init__(self, port=6502):
        self.socket = socket.create_connection(("127.0.0.1", port), timeout=15)
        self.socket.settimeout(15)
        self.request = 0

    def receive(self):
        def exact(n):
            data = bytearray()
            while len(data) < n:
                part = self.socket.recv(n - len(data))
                if not part:
                    raise EOFError("Reference emulator disconnected")
                data.extend(part)
            return data
        header = exact(12)
        if header[0] != 2:
            raise ValueError("Invalid VICE response header")
        length, kind, error, request = struct.unpack_from("<IBBI", header, 2)
        return kind, error, request, bytes(exact(length))

    def command(self, kind, body=b""):
        self.request += 1
        self.socket.sendall(struct.pack("<BBIIB", 2, 2, len(body), self.request, kind) + body)
        while True:
            response, error, request, data = self.receive()
            if request == self.request:
                if error:
                    raise RuntimeError(f"VICE {kind:02x} error {error:02x}")
                return data

    def memory(self, start=0, end=0xffff, bank=1):
        return self.command(1, struct.pack("<BHHBH", 0, start, end, 0, bank))[2:]

    def write(self, address, data, bank=1):
        return self.command(2, struct.pack("<BHHBH", 0, address, address + len(data) - 1, 0, bank) + data)

    def checkpoint(self, address, temporary=True):
        return self.command(0x12, struct.pack("<HHBBBBB", address, address, 1, 1, 4, temporary, 0))

    def resume(self):
        self.command(0xaa)

    def wait_stopped(self):
        while True:
            kind, error, request, data = self.receive()
            if kind == 0x62:
                return int.from_bytes(data[:2], "little")

    def pc(self, value):
        self.command(0x32, struct.pack("<BHB BH", 0, 1, 3, 3, value))

    def snapshot(self, filename):
        name = str(Path(filename).resolve()).replace("\\", "/").encode("ascii")
        self.command(0x41, bytes([0, 0, len(name)]) + name)

    def close(self):
        self.socket.close()
