"""Mix original SID capture, the game's guitar arrangement and film impacts."""
from pathlib import Path
import sys,wave,json,subprocess,re
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'.tools/python'))
import numpy as np
WORK=ROOT/'release-work/trailer-1.2';RATE=48000;SECONDS=75;N=RATE*SECONDS
rng=np.random.default_rng(6412)
def load(path):
 with wave.open(str(path),'rb') as w:
  assert w.getframerate()==RATE and w.getsampwidth()==2
  a=np.frombuffer(w.readframes(w.getnframes()),'<i2').astype(np.float64).reshape(-1,w.getnchannels())/32768
  return np.repeat(a,2,axis=1) if a.shape[1]==1 else a
def save(path,a):
 with wave.open(str(path),'wb') as w:w.setnchannels(2);w.setsampwidth(2);w.setframerate(RATE);w.writeframes((np.clip(a,-.999,.999)*32767).astype('<i2').tobytes())
mix=np.zeros((N,2),np.float64);time=np.arange(N)/RATE
def add(a,at,gain=1):
 start=round(at*RATE);a=a[max(0,-start):];start=max(0,start);length=min(len(a),N-start)
 if length>0:mix[start:start+length]+=a[:length]*gain
sid=load(WORK/'original-sid-wmm.wav');st=np.arange(len(sid))/RATE
sid*=np.minimum(st/.035,1)[:,None]*np.clip((6-st)/.35,0,1)[:,None]
add(sid,0,2.25)
music=load(ROOT/'Content/Audio/course1-studio.wav');mask=time>=6;relative=time[mask]-6
musicindices=(relative*RATE).astype(np.int64)%len(music);bed=music[musicindices].copy()
# The existing recording alternates raspy and overdriven phrases. Its second
# half supplies the heavier final section, retaining the recorded melody.
drive=time[mask]>=52;drive_start=len(music)//2;drive_length=len(music)-drive_start
drive_indices=drive_start+((time[mask][drive]-52)*RATE).astype(np.int64)%drive_length
bed[drive]=music[drive_indices]
gain=np.minimum(relative/.15,1)*np.clip((74-time[mask])/1.4,0,1)*.86
for at in [12+2/3,28.5,64.1,68+2/3]:gain*=1-.45*np.exp(-((time[mask]-at)/.24)**2)
mix[mask]+=bed*gain[:,None]
for name,volume,start,end in [('park',.23,6,68),('rolling',.11,10.1,68)]:
 a=load(ROOT/f'Content/Audio/{name}.wav');active=(time>=start)&(time<end);indices=(np.arange(active.sum())%len(a));mix[active]+=a[indices]*volume
def whoosh(duration=.7):
 n=round(duration*RATE);t=np.arange(n)/RATE;x=rng.normal(0,1,n)
 freqs=np.fft.rfftfreq(n,1/RATE);x=np.fft.irfft(np.fft.rfft(x)*(freqs/(freqs+300))/(1+(freqs/7000)**4),n)
 x*=np.sin(np.pi*t/duration)**1.8*.13
 pan=np.linspace(0,1,n)
 return np.column_stack([x*(1-pan*.75),x*(.25+pan*.75)])
def impact(duration=1.5):
 n=round(duration*RATE);t=np.arange(n)/RATE
 bass=.48*np.sin(2*np.pi*(36*t+3.8*(1-np.exp(-t*38))))*np.exp(-t*7)
 air=rng.normal(0,1,n)*np.exp(-t*29)*.18
 metal=sum(np.sin(2*np.pi*f*t)*np.exp(-t*d)*g for f,d,g in [(181,8,.07),(479,13,.05),(1267,17,.025),(2871,25,.012)])
 chord=sum(sum(np.sin(2*np.pi*f*h*t)*np.exp(-t*(1.7+h*.2))/h for h in range(1,13)) for f in [82.4069,123.4708,164.8138])
 chord=np.tanh(chord*2.8)*np.minimum(t/.004,1)*np.exp(-t*2)*.16
 v=bass+air+metal+chord
 return np.column_stack([v,np.roll(v,17)])
add(whoosh(.7),5.65,.9)
for at,volume in [(12+2/3,1),(68+2/3,.88)]:
 add(whoosh(2/3),at-2/3,1.1);add(impact(),at,volume)
for at in [17,18,19,20,26,28,33,35,37,39,41,43,45,47.5,49,52,56,60,62.4,64.5]:add(whoosh(.14),at-.07,.28)
add(load(ROOT/'Content/Audio/crash.wav'),28.5,.83)
add(load(ROOT/'Content/Audio/start.wav'),31.12,.35)
for at in [33,35,37,39,41,43,47.5]:add(load(ROOT/'Content/Audio/select.wav'),at,.32)
add(load(ROOT/'Content/Audio/waterside.wav')[:RATE*4],52,.33)
add(load(ROOT/'Content/Audio/finish.wav'),64.12,.75)
for i,at in enumerate([64.7,65.4,66.3,67.2,69.5,70.3,71.3]):
 a=load(ROOT/'Content/Audio/firework.wav');a[:,i%2]*=1.1;a[:,1-i%2]*=.7;add(a,at,.53)
tail=impact(2.1);tail*=np.linspace(1,0,len(tail))[:,None];add(tail,73.05,.28)
mix*=np.clip((SECONDS-time)/.5,0,1)[:,None]
mix=np.tanh(mix*.86)*.95
raw=WORK/'trailer-mix-raw.wav';save(raw,mix)
final=WORK/'trailer-mix.wav'
cmd=['C:/dev/ffmpeg/bin/ffmpeg.exe','-hide_banner','-y','-i',str(raw),'-af','loudnorm=I=-14:TP=-1.5:LRA=9:print_format=json','-ar','48000','-c:a','pcm_s24le',str(final)]
r=subprocess.run(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,check=True)
(WORK/'audio-normalization.log').write_text(r.stderr,encoding='utf-8')
measure=json.loads(re.search(r'\{[^{}]*"input_i"[^{}]*\}',r.stderr,re.S).group())
filter2='loudnorm=I=-14:TP=-1.5:LRA=9:measured_I='+measure['input_i']+':measured_TP='+measure['input_tp']+':measured_LRA='+measure['input_lra']+':measured_thresh='+measure['input_thresh']+':offset='+measure['target_offset']+':linear=false:print_format=json'
r=subprocess.run(['C:/dev/ffmpeg/bin/ffmpeg.exe','-hide_banner','-y','-i',str(raw),'-af',filter2,'-ar','48000','-c:a','pcm_s24le',str(final)],stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,check=True)
(WORK/'audio-normalization-final.log').write_text(r.stderr,encoding='utf-8')
print('75-second stereo mix completed:',final)
