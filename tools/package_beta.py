"""Create a clean, portable beta ZIP without developer saves, logs or symbols."""
from pathlib import Path
import hashlib, json, shutil, zipfile, sys, re, argparse

ROOT = Path(__file__).resolve().parents[1]
VERSION = re.search(r'^ProjectVersion=(.+)$',(ROOT/'Config/DefaultGame.ini').read_text(encoding='utf-8'),re.M).group(1).strip()
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('source',nargs='?')
parser.add_argument('--edition',help='Unique local preview suffix; never replaces a published version.')
args=parser.parse_args()
if args.edition:
    assert re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9.-]{0,60}',args.edition),'Invalid edition name'
    VERSION+='-'+args.edition
OUT = ROOT / 'releases' / VERSION
STAGE = OUT / ('BMX-Cruisers-' + VERSION + '-Windows-x64')
assert not STAGE.exists(), 'Use a new release directory; never overwrite a staged release.'
STAGE.mkdir(parents=True)
source = Path(args.source).resolve() if args.source else ROOT / 'Build/Windows'
assert (source / 'BMXCruiser.exe').is_file(), 'Supply a packaged Windows directory.'
for path in source.rglob('*'):
    if not path.is_file(): continue
    rel = path.relative_to(source)
    if any(part.lower() in {'saved', 'intermediate', 'metadata'} for part in rel.parts): continue
    if path.suffix.lower() in {'.pdb', '.log', '.dmp', '.sav', '.txt'} and path.name not in {'CREDITS.txt', 'LICENSE-Rocketbox.txt', 'LICENSE-Roboto.txt', 'NOTICES.txt'}: continue
    target = STAGE / rel
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(path, target)
for src, name in [('docs/PLAY.md' if (ROOT/'docs/PLAY.md').exists() else 'docs/RELEASE-1.0-BETA.md', 'START-HERE.txt'), ('THIRD-PARTY-NOTICES.md', 'THIRD-PARTY-NOTICES.txt'), ('CHANGELOG.md', 'RELEASE-NOTES.txt'), ('assets/CREDITS.md', 'CREDITS.txt')]:
    shutil.copy2(ROOT / src, STAGE / name)
if args.edition and (ROOT/'docs/PERFORMANCE.md').exists():
    shutil.copy2(ROOT/'docs/PERFORMANCE.md',STAGE/'PERFORMANCE.txt')
license_text = (ROOT / 'Source/BMXCruiser/ThirdParty/m6502.h').read_text(encoding='utf-8')
# Retain the complete original copyright/license header, including its usage notes.
(STAGE / 'LICENSE-6502.txt').write_text(license_text.split('#include')[0], encoding='utf-8')
redist = Path('C:/Program Files/Epic Games/UE_5.8/Engine/Extras/Redist/en-us/vc_redist.x64.exe')
target = STAGE / 'Engine/Extras/Redist/en-us/vc_redist.x64.exe'
target.parent.mkdir(parents=True, exist_ok=True)
shutil.copy2(redist, target)
(STAGE / 'VERSION.txt').write_text(VERSION + '\n', encoding='utf-8')
manifest = [{'path': str(p.relative_to(STAGE)).replace('\\', '/'), 'bytes': p.stat().st_size, 'sha256': hashlib.sha256(p.read_bytes()).hexdigest()} for p in sorted(STAGE.rglob('*')) if p.is_file()]
(STAGE / 'MANIFEST.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
archive = OUT / (STAGE.name + '.zip')
with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as z:
    for p in sorted(STAGE.rglob('*')):
        if p.is_file(): z.write(p, p.relative_to(OUT))
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
digest = hashlib.sha256(archive.read_bytes()).hexdigest()
(OUT / 'SHA256SUMS.txt').write_text(digest + '  ' + archive.name + '\n', encoding='utf-8')
print(json.dumps({'file': str(archive), 'bytes': archive.stat().st_size, 'sha256': digest, 'files': len(manifest)}, indent=2))
