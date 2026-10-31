"""Extract the PRG from this project's D64, validating every linked sector."""
from pathlib import Path
import hashlib
import json

ROOT = Path(__file__).resolve().parents[1]


def sector_offset(track, sector):
    def count(t):
        return 21 if t <= 17 else 19 if t <= 24 else 18 if t <= 30 else 17
    if not 1 <= track <= 35 or not 0 <= sector < count(track):
        raise ValueError(f"Invalid track/sector {track}/{sector}")
    return (sum(count(t) for t in range(1, track)) + sector) * 256


def extract(disk):
    track, sector = 18, 1
    seen = set()
    while track:
        if (track, sector) in seen:
            raise ValueError("Directory loop")
        seen.add((track, sector))
        offset = sector_offset(track, sector)
        for entry in range(8):
            e = offset + entry * 32
            if disk[e + 2] & 7 != 2:
                continue
            name = bytes(disk[e + 5:e + 21]).rstrip(b"\xa0").decode("ascii")
            t, s = disk[e + 3:e + 5]
            blocks = int.from_bytes(disk[e + 30:e + 32], "little")
            chain, payload = set(), bytearray()
            while t:
                if (t, s) in chain:
                    raise ValueError("File sector loop")
                chain.add((t, s))
                o = sector_offset(t, s)
                nt, ns = disk[o:o + 2]
                payload.extend(disk[o + 2:o + (ns + 1 if nt == 0 else 256)])
                t, s = nt, ns
            if len(chain) != blocks:
                raise ValueError("File block count mismatch")
            yield name, bytes(payload), blocks
        track, sector = disk[offset:offset + 2]


if __name__ == "__main__":
    source = ROOT / "bmxracer.d64"
    disk = source.read_bytes()
    output = ROOT / "reference/extracted"
    output.mkdir(parents=True, exist_ok=True)
    files = []
    for name, payload, blocks in extract(disk):
        filename = "bmx-racer.prg"
        (output / filename).write_bytes(payload)
        files.append(dict(name=name, file=filename, blocks=blocks, bytes=len(payload),
                          load_address=int.from_bytes(payload[:2], "little"),
                          sha256=hashlib.sha256(payload).hexdigest()))
    manifest = dict(source=source.name, sha256=hashlib.sha256(disk).hexdigest(), files=files)
    (output / "disk-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps(manifest, indent=2))
