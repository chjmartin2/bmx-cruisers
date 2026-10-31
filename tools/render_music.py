"""Produce the original melody in a four-phrase studio synth arrangement and FX.

The note sequence and SID frequency table are read from the user's game image.
00 is a tie, FD a rest, and FA the loop marker. Five 60 Hz IRQs advance a slot.
"""
from pathlib import Path
import json, wave
import numpy as np

ROOT=Path(__file__).resolve().parents[1]
RATE=48000


def render():
    ram=(ROOT/'reference/extracted/course1-initial-ram.bin').read_bytes()
    sequence=[]
    for value in ram[0x4000:0x4100]:
        sequence.append(value)
        if value==250:break
    assert sequence[-1]==250
    step=5/60
    # FA is an immediate jump, not an extra timed note slot.
    period=(len(sequence)-1)*step
    size=round(period*RATE);audio=np.zeros((size*6,2),dtype=np.float64)
    random=np.random.default_rng(1985)
    events=[]
    for index,note in enumerate(sequence):
        if note==0 or note>=128:continue
        end=index+1
        while end<len(sequence) and sequence[end]==0:end+=1
        frequency=(ram[0xc300+note]+256*ram[0xc340+note])*985248/(2**24)
        duration=(end-index)*step-1/60
        events.append(dict(slot=index,note_index=note,frequency=frequency,duration=duration))
        for cycle in range(6):
            offset=cycle*size+round(index*step*RATE);length=min(round((duration+.16)*RATE),len(audio)-offset)
            t=np.arange(length)/RATE
            attack=np.minimum(t/.009,1)
            release=np.exp(-np.maximum(t-duration,0)/.045)
            envelope=attack*(.35+.65*np.exp(-t/1.1))*release
            # Plucked-string excitation, pick transient and a guitar cabinet impulse.
            # The same recovered notes alternate clean/raspy and heavily driven phrases.
            driven=cycle%4 in [2,3]
            for channel,detune in enumerate([-.0007,.0007]):
                phase=2*np.pi*frequency*(1+detune)*t+.004*np.sin(2*np.pi*5.1*t)*np.minimum(t/.08,1)
                string=sum(((-1)**(h+1))*np.sin(h*phase)*np.exp(-t*(1.4+h*.45))/h**.85 for h in range(1,19))
                pick=random.normal(0,1,length)*np.exp(-t*230)*.1
                raw=(string+pick)*envelope
                tone=np.tanh(raw*(6.8 if driven else 1.65))
                # Guitar cabinet: roll off the brittle top above 5 kHz and the sub bass.
                freqs=np.fft.rfftfreq(length,1/RATE)
                filt=(freqs/(freqs+75))**2/(1+(freqs/(4300 if driven else 5700))**6)
                filt*=1+.35*np.exp(-((freqs-1900)/950)**2)
                tone=np.fft.irfft(np.fft.rfft(tone)*filt,length)
                audio[offset:offset+length,channel]+=(.145 if driven else .19)*tone
    # At this phrase speed a half-time 90 BPM rhythm leaves the melody room.
    beat=step*8
    for cycle in range(6):
        for index in range(int(np.ceil(period/beat))):
            offset=cycle*size+round(index*beat*RATE)
            length=min(round(.32*RATE),len(audio)-offset)
            if length<=0:continue
            t=np.arange(length)/RATE
            if index%4 in [0,2]:
                kick=np.sin(2*np.pi*(47*t+7*(1-np.exp(-t*35))))*np.exp(-t*16)
                audio[offset:offset+length]+=(.22*kick)[:,None]
            else:
                noise=random.normal(0,1,length);noise=np.concatenate(([0],np.diff(noise)))
                snare=(.07*noise*np.exp(-t*24)+.065*np.sin(2*np.pi*175*t)*np.exp(-t*20))
                audio[offset:offset+length]+=snare[:,None]
            for offbeat in [0,.5]:
                start=offset+round(offbeat*beat*RATE);n=min(round(.07*RATE),len(audio)-start)
                if n>0:
                    tt=np.arange(n)/RATE;noise=random.normal(0,1,n)
                    hat=np.concatenate(([0],np.diff(noise)))*np.exp(-tt*80)*.018
                    audio[start:start+n,1]+=.8*hat;audio[start:start+n,0]+=.6*hat
        # Tuned bass and soft wide chords support the recovered lead. Phrase
        # variations and bar-end fills keep the arrangement from feeling static.
        bar=beat*4
        for index,root in enumerate([211.9/4,168.2/4,252.0/4,188.8/4]):
            chord_start=cycle*size+round(index*bar*RATE)
            n=min(round((bar+.3)*RATE),len(audio)-chord_start)
            t=np.arange(n)/RATE;env=np.minimum(t/.12,1)*np.minimum(np.maximum(bar+.3-t,0)/.4,1)
            intervals=[0,3 if index==0 else 4,7]
            for channel,detune in enumerate([.998,1.002]):
                pad=sum(np.sin(2*np.pi*root*4*2**(interval/12)*detune*t) for interval in intervals)
                audio[chord_start:chord_start+n,channel]+=.021*pad*env*(.85+.15*np.sin(t*2.3+channel))
            for eighth in range(8):
                offset=chord_start+round(eighth*beat*.5*RATE)
                n=min(round(.31*RATE),len(audio)-offset)
                if n<=0:continue
                t=np.arange(n)/RATE;frequency=root*(2 if eighth in [3,7] else 1)
                bass=(np.sin(2*np.pi*frequency*t)+.27*np.sin(4*np.pi*frequency*t)+.07*np.sin(6*np.pi*frequency*t))
                env=np.minimum(t/.005,1)*np.exp(-t*12)
                audio[offset:offset+n]+=(.16*bass*env)[:,None]
            if cycle%4 in [2,3]:
                for fill in range(4):
                    offset=chord_start+round((bar-beat+fill*beat/4)*RATE)
                    n=min(round(.09*RATE),len(audio)-offset)
                    if n>0:
                        t=np.arange(n)/RATE;noise=random.normal(0,1,n)
                        audio[offset:offset+n]+=(.023*noise*np.exp(-t*42))[:,None]
    # Stereo echo and a short diffuse tail are folded into the repeating phrase.
    dry=audio.copy()
    for seconds,gain in [(step*3,.15),(.113,.07),(.179,.05),(.263,.035),(.397,.025)]:
        delay=round(seconds*RATE)
        audio[delay:,0]+=dry[:-delay,1]*gain
        audio[delay:,1]+=dry[:-delay,0]*gain
    take=audio[size:5*size]
    take=np.tanh(take*1.25);take*=.82/max(float(np.max(np.abs(take))),1e-9)
    pcm=(take*32767).astype('<i2').tobytes()
    output=ROOT/'Content/Audio';output.mkdir(parents=True,exist_ok=True)
    (output/'course1-studio.pcm').write_bytes(pcm)
    with wave.open(str(output/'course1-studio.wav'),'wb') as file:
        file.setnchannels(2);file.setsampwidth(2);file.setframerate(RATE);file.writeframes(pcm)
    (ROOT/'reference/extracted/music-notes.json').write_text(json.dumps(dict(
        sequence_address='0x4000',frequency_addresses=['0xc300','0xc340'],
        slots=sequence,slot_seconds=step,phrase_seconds=period,loop_seconds=period*4,events=events),indent=2)+'\n')
    print(f'Stereo electric-guitar arrangement: {period*4:.3f}s, four phrases, {len(events)} recovered notes, {RATE} Hz.')

def sound_effects():
    random=np.random.default_rng(1979)
    output=ROOT/'Content/Audio'
    def noise(seconds,low,high):
        n=round(seconds*RATE);frequencies=np.fft.rfftfreq(n,1/RATE)
        spectrum=np.fft.rfft(random.normal(size=n))
        spectrum*=np.clip((frequencies-low)/max(low,50),0,1)*np.clip((high-frequencies)/max(high*.3,100),0,1)
        signal=np.fft.irfft(spectrum,n);return signal/max(np.std(signal),1e-6)
    def save(name,a):
        if a.ndim==1:a=np.column_stack([a,a])
        a=np.tanh(a);a*=min(1,.90/max(np.max(np.abs(a)),1e-6))
        pcm=(a*32767).astype('<i2').tobytes();(output/f'{name}.pcm').write_bytes(pcm)
        with wave.open(str(output/f'{name}.wav'),'wb') as f:f.setnchannels(2);f.setsampwidth(2);f.setframerate(RATE);f.writeframes(pcm)
        return dict(name=name,seconds=len(a)/RATE,peak=float(np.max(np.abs(a))),rms=float(np.sqrt(np.mean(a*a))))
    result=[]
    t=np.arange(RATE*6)/RATE
    roll=noise(6,90,3200)*(.023+.006*np.sin(t*2*np.pi*.5))
    for at in np.arange(0,6,1/7):
        start=round(at*RATE);n=min(round(.027*RATE),len(t)-start);tt=np.arange(n)/RATE
        roll[start:start+n]+=(.038*np.sin(2*np.pi*2370*tt)+.015*np.sin(2*np.pi*4130*tt))*np.exp(-tt*210)
    result.append(save('rolling',roll))
    t=np.arange(RATE*20)/RATE
    park=noise(20,140,1900)*(.013+.006*np.sin(2*np.pi*t/20))
    stereo=np.column_stack([park,np.roll(park,347)])
    for at in [1.1,2.8,6.5,10.3,14.1,17.9]:
        start=round(at*RATE);n=round(.31*RATE);tt=np.arange(n)/RATE
        bird=np.sin(2*np.pi*(2800*tt+330*tt**2+22*np.sin(tt*34)))*np.sin(np.pi*tt/.31)**2*.012
        stereo[start:start+n,int(at)%2]+=bird
    result.append(save('park',stereo))
    water=noise(20,180,3700)*(.023+.013*np.sin(t*2.1)**2)
    for at in np.arange(.3,19.5,.47):
        start=round(at*RATE);n=min(round(.15*RATE),len(t)-start);tt=np.arange(n)/RATE
        water[start:start+n]+=.018*np.sin(2*np.pi*(950*tt-1400*tt*tt))*np.exp(-tt*27)
    result.append(save('waterside',stereo+np.column_stack([water,np.roll(water,117)])))
    def bell(seconds,frequency):
        t=np.arange(round(seconds*RATE))/RATE
        return sum(gain*np.sin(2*np.pi*frequency*ratio*t)*np.exp(-t*decay) for ratio,gain,decay in [(1,.28,3),(2.756,.12,7),(5.404,.04,15)])*np.minimum(t/.002,1)
    result.append(save('start',bell(1.1,1510)))
    result.append(save('select',bell(.10,920)*.16))
    t=np.arange(round(1.45*RATE))/RATE
    impact=.27*noise(1.45,35,2200)*np.exp(-t*12)+.24*np.sin(2*np.pi*65*t)*np.exp(-t*24)
    skid=.09*noise(1.45,900,6500)*np.exp(-t*4)*np.minimum(t/.015,1)
    clatter=.06*(np.sin(2*np.pi*1370*t)+np.sin(2*np.pi*2849*t))*np.exp(-t*8)*(np.sin(t*47)**6)
    result.append(save('crash',impact+skid+clatter))
    splash=.20*noise(1.45,60,4500)*np.exp(-t*3)*np.minimum(t/.012,1)+impact*.45
    result.append(save('splash',splash))
    finish=np.zeros(RATE*2)
    for at,f in [(0,635.1),(.14,755.2),(.28,847.6),(.42,1270.2)]:
        start=round(at*RATE);tt=np.arange(round(1.2*RATE))/RATE;string=sum(np.sin(2*np.pi*f*h*tt)*np.exp(-tt*(3+h*.5))/h for h in range(1,13));part=np.tanh(string*4)*np.minimum(tt/.003,1)*np.exp(-tt*2)*.2;finish[start:start+len(part)]+=part
    result.append(save('finish',finish))
    t=np.arange(round(.65*RATE))/RATE
    pop=.23*noise(.65,70,4200)*np.exp(-t*22)+.08*noise(.65,900,6000)*np.exp(-t*7)*(np.sin(t*83)**8)
    result.append(save('firework',pop))
    (ROOT/'reference/extracted/audio-production.json').write_text(json.dumps(dict(sample_rate=RATE,channels=2,origin='Authored plucked-string electric guitar, clean/raspy and overdrive cabinet processing, bass/drums and foley; recovered original note sequence; no SID recording',effects=result),indent=2)+'\n')
    print('Sound design:',', '.join(item['name'] for item in result))


if __name__=='__main__':render();sound_effects()
