"""Fetch a pinned, MIT-licensed Rocketbox character and retain provenance."""
from pathlib import Path
import urllib.request,json,hashlib,concurrent.futures
ROOT=Path(__file__).resolve().parents[1]
tree=json.loads((ROOT/'assets/rocketbox-tree.json').read_text(encoding='utf-8-sig'))
revision=tree['sha']
prefix='Assets/Avatars/Adults/Male_Adult_01/'
entries=[entry for entry in tree['tree'] if entry['path'].startswith(prefix) and
         (entry['path'].endswith('/Male_Adult_01.fbx') or '/Textures/' in entry['path']) and entry['type']=='blob']
def fetch(entry):
    path=entry['path'];target=ROOT/'assets/source/rocketbox/Male_Adult_01'/Path(path).name
    target.parent.mkdir(parents=True,exist_ok=True)
    url=f'https://raw.githubusercontent.com/microsoft/Microsoft-Rocketbox/{revision}/{urllib.parse.quote(path)}'
    if not target.exists() or target.stat().st_size!=entry['size']:
        urllib.request.urlretrieve(url,target)
    data=target.read_bytes()
    gitsha=hashlib.sha1(f'blob {len(data)}\0'.encode()+data).hexdigest()
    if gitsha!=entry['sha']:raise ValueError(f'Character checksum mismatch: {target.name}')
    return dict(file=str(target.relative_to(ROOT)),url=url,sha256=hashlib.sha256(data).hexdigest())
with concurrent.futures.ThreadPoolExecutor(max_workers=5) as pool:files=list(pool.map(fetch,entries))
(ROOT/'assets/rider-manifest.json').write_text(json.dumps(dict(source='https://github.com/microsoft/Microsoft-Rocketbox',revision=revision,license='MIT',files=files),indent=2)+'\n')
print(f'Character: {len(files)} files verified at {revision}')
