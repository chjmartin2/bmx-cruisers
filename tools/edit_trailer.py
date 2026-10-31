"""Edit native 30fps Unreal frame sequences into a YouTube-ready beta trailer."""
from pathlib import Path
import subprocess, json

ROOT = Path(__file__).resolve().parents[1]
WORK = ROOT / 'release-work'
OUT = ROOT / 'releases/1.0.0-beta.1'
OUT.mkdir(parents=True, exist_ok=True)
FFMPEG = r'C:\dev\ffmpeg\bin\ffmpeg.exe'
FONT = 'C\\:/Windows/Fonts/arialbd.ttf'
clips = [
    ('title', 60, 4, '1.0 BETA  /  WINDOWS'),
    ('ride', 60, 14, 'C64 COURSE ONE. A NEW VIEW.'),
    ('garage-bmx', 60, 4, 'MAKE THE RIDE YOURS'),
    ('garage-inline', 60, 4, 'SIX STYLES. THE SAME CHALLENGE.'),
    ('garage-wheelchair', 60, 4, 'CHOOSE YOUR RIDER. PICK YOUR COLORS.'),
    ('ride', 480, 11.5, 'FIND YOUR LINE. REACH THE FINISH.'),
    ('title', 15, 5.5, 'PLAY THE WINDOWS BETA  /  RETROCOMPUTERIST.COM'),
]
def run(args):
    subprocess.run([FFMPEG, '-hide_banner', '-loglevel', 'warning', '-y', *args], check=True)

for i, (shot, frame, seconds, text) in enumerate(clips):
    # Titles replace the control hint area, leaving all gameplay visible above it.
    vf = f"drawbox=x=0:y=930:w=iw:h=150:color=0x111a28@0.96:t=fill,drawbox=x=0:y=930:w=iw:h=4:color=0xffef24:t=fill,drawtext=fontfile='{FONT}':text='{text}':fontsize=38:fontcolor=0xffef24:x=(w-tw)/2:y=963,drawtext=fontfile='{FONT}':text='BMX CRUISERS  |  COURSE ONE':fontsize=21:fontcolor=white:x=(w-tw)/2:y=1023,fade=t=in:st=0:d=0.15,fade=t=out:st={seconds-.15}:d=0.15"
    run(['-framerate', '30', '-start_number', str(frame), '-i', str(WORK / 'frames' / shot / 'frame-%06d.png'), '-t', str(seconds), '-vf', vf, '-an', '-c:v', 'libx264', '-threads', '4', '-crf', '18', '-preset', 'medium', '-pix_fmt', 'yuv420p', '-r', '30', str(WORK / f'clip-{i}.mp4')])
    print(f'Encoded clip {i}: {shot}', flush=True)
concat = WORK / 'trailer-concat.txt'
concat.write_text(''.join("file '" + (WORK / f'clip-{i}.mp4').as_posix() + "'\n" for i in range(len(clips))), encoding='utf-8')
silent = WORK / 'trailer-picture.mp4'
run(['-f', 'concat', '-safe', '0', '-i', str(concat), '-c', 'copy', str(silent)])
duration = sum(c[2] for c in clips)
# Use the game's authored guitar arrangement and environmental effects as a trailer mix.
audio = ROOT / 'Content/Audio'
mix = '[1:a]volume=0.8[m];[2:a]volume=0.15[p];[3:a]volume=0.55,adelay=37200|37200[f];[4:a]volume=0.45,adelay=39000|39000[w];[m][p][f][w]amix=inputs=4:duration=longest:normalize=0,loudnorm=I=-16:TP=-1.5:LRA=11,afade=t=in:d=0.4,afade=t=out:st=44:d=3[a]'
final = OUT / 'BMX-Cruisers-1.0-Beta-Trailer-1080p.mp4'
run(['-i', str(silent), '-stream_loop', '-1', '-i', str(audio / 'course1-studio.wav'), '-stream_loop', '-1', '-i', str(audio / 'park.wav'), '-i', str(audio / 'finish.wav'), '-i', str(audio / 'firework.wav'), '-filter_complex', mix, '-map', '0:v', '-map', '[a]', '-t', str(duration), '-c:v', 'copy', '-c:a', 'aac', '-b:a', '320k', '-ar', '48000', '-movflags', '+faststart', str(final)])
print(json.dumps({'trailer': str(final), 'duration_seconds': duration, 'resolution': '1920x1080', 'fps': 30, 'audio': 'AAC stereo, 48kHz'}, indent=2))
