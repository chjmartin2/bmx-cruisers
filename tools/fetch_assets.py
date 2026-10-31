"""Fetch a small, attributed set of CC0 Poly Haven source assets, checking hashes."""
from pathlib import Path
from urllib.request import Request, urlopen
import hashlib
import json

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / 'assets/source'
HEADERS = {'User-Agent': 'BMXCruiserDevelopment/0.1 (local asset preparation)'}


def download(item, path):
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.exists() or hashlib.md5(path.read_bytes()).hexdigest() != item['md5']:
        with urlopen(Request(item['url'], headers=HEADERS), timeout=60) as response:
            path.write_bytes(response.read())
    if hashlib.md5(path.read_bytes()).hexdigest() != item['md5']:
        raise ValueError(f'Asset checksum mismatch: {path.name}')
    return dict(file=str(path.relative_to(ROOT)), url=item['url'],
                sha256=hashlib.sha256(path.read_bytes()).hexdigest())


if __name__ == '__main__':
    manifest = []
    for name in ['shrub_01', 'grass_bermuda_01', 'asphalt_02', 'grass_ground']:
        metadata = json.loads((OUTPUT / f'{name}-files.json').read_text(encoding='utf-8-sig'))
        size = '1k' if 'fbx' in metadata else '2k'
        items = []
        if 'fbx' in metadata:
            model = metadata['fbx'][size]['fbx']
            items.append(download(model, OUTPUT / name / f'{name}.fbx'))
        for key in ['Diffuse', 'nor_dx', 'Rough', 'Alpha']:
            if key in metadata:
                formats = metadata[key][size]
                ext = 'png' if key == 'Alpha' else 'jpg'
                item = formats[ext]
                items.append(download(item, OUTPUT / name / f'{key}.{ext}'))
        manifest.append(dict(asset=name, source=f'https://polyhaven.com/a/{name}',
                             license='CC0-1.0', files=items))
        print(f'{name}: {len(items)} files verified', flush=True)
    (ROOT / 'assets/manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
