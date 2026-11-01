"""Package and verify the finished trailer without publishing it."""
from pathlib import Path
import subprocess, json, sys, hashlib
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'.tools/python'))
import cv2
from PIL import Image, ImageDraw, ImageFont
WORK=ROOT/'release-work/trailer-1.2'
OUT=ROOT/'releases/1.2.0/media'
FF='C:/dev/ffmpeg/bin/ffmpeg.exe'
FP='C:/dev/ffmpeg/bin/ffprobe.exe'
def run(args,log):
 with (WORK/log).open('w',encoding='utf8') as f:subprocess.run(args,stdout=f,stderr=subprocess.STDOUT,check=True)
def probe(path):
 return json.loads(subprocess.check_output([FP,'-v','error','-show_streams','-show_format','-of','json',str(path)]))
def atoms(path):
 result=[]
 with path.open('rb') as f:
  while True:
   head=f.read(8)
   if not head:break
   size=int.from_bytes(head[:4],'big');kind=head[4:].decode('ascii');header=8
   if size==1:size=int.from_bytes(f.read(8),'big');header=16
   result.append(kind)
   if size==0:break
   f.seek(size-header,1)
 return result
def main():
 OUT.mkdir(parents=True,exist_ok=True)
 master=OUT/'BMX-Cruisers-1.2-Trailer-4K.mp4'
 share=OUT/'BMX-Cruisers-1.2-Trailer-1080p.mp4'
 run([FF,'-hide_banner','-y','-i',str(WORK/'trailer-picture-4K.mp4'),'-i',str(WORK/'trailer-mix.wav'),'-map','0:v:0','-map','1:a:0','-c:v','copy','-c:a','aac','-b:a','320k','-ar','48000','-t','75','-movflags','+faststart','-metadata','title=BMX Cruisers | Version 1.2 Trailer','-metadata','comment=Original C64 emulator capture and actual Windows gameplay; editorial transformation and logo animation.',str(master)],'master-mux.log')
 print('4K master packaged',flush=True)
 run([FF,'-hide_banner','-y','-i',str(master),'-map','0:v:0','-map','0:a:0','-vf','scale=1920:1080:flags=lanczos','-c:v','h264_nvenc','-preset','p6','-rc','vbr','-cq','19','-b:v','0','-spatial-aq','1','-temporal-aq','1','-pix_fmt','yuv420p','-color_primaries','bt709','-color_trc','bt709','-colorspace','bt709','-c:a','copy','-movflags','+faststart',str(share)],'sharing-encode.log')
 print('1080p sharing copy packaged',flush=True)
 reports={}
 for path,width in [(master,3840),(share,1920)]:
  report=probe(path);video=next(s for s in report['streams'] if s['codec_type']=='video');audio=next(s for s in report['streams'] if s['codec_type']=='audio')
  assert (video['width'],video['height'])==(width,width*9//16)
  assert video['codec_name']=='h264' and video['pix_fmt']=='yuv420p' and video['r_frame_rate']=='30/1'
  assert int(video['nb_frames'])==2250 and abs(float(report['format']['duration'])-75)<.05
  assert audio['codec_name']=='aac' and audio['channels']==2 and audio['sample_rate']=='48000'
  run([FF,'-v','error','-i',str(path),'-f','null','-'],f'{width}-full-decode.log')
  assert not (WORK/f'{width}-full-decode.log').read_text().strip()
  boxes=atoms(path);assert boxes.index('moov')<boxes.index('mdat')
  reports[path.name]={'probe':report,'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'full_decode':'passed','web_fast_start':True}
  print(f'{width}px file: duration, streams, frames and full decode passed',flush=True)
 run([FF,'-hide_banner','-i',str(share),'-vn','-af','loudnorm=I=-14:TP=-1.5:LRA=9:print_format=json','-f','null','-'],'final-aac-audio-analysis.log')
 # Contact sheet from the actual final encoded file, including every scene.
 cap=cv2.VideoCapture(str(share));sheet=Image.new('RGB',(1600,6*205),(15,18,22));draw=ImageDraw.Draw(sheet)
 times=[.8,3.5,7.4,10.5,12.7,13.4,16.5,18.5,20.5,22,27,29,32.7,34.1,36.1,38.1,40.1,42.1,44.1,46,48,50,54,58,61,63,66,68.7,70.5,74]
 font=ImageFont.truetype(str(ROOT/'Content/UI/Fonts/Roboto-Medium.ttf'),18)
 for i,t in enumerate(times):
  cap.set(cv2.CAP_PROP_POS_MSEC,t*1000);ok,frame=cap.read();assert ok
  p=Image.fromarray(cv2.cvtColor(frame,cv2.COLOR_BGR2RGB));p.thumbnail((320,180));x=i%5*320;y=i//5*205
  sheet.paste(p,(x,y));draw.text((x+8,y+183),f'{t:05.2f}s',font=font,fill='white')
 cap.release();sheet.save(OUT/'BMX-Cruisers-1.2-Trailer-Contact-Sheet.jpg',quality=94)
 (OUT/'Trailer-Verification.json').write_text(json.dumps(reports,indent=2),encoding='utf8')
 (OUT/'SHA256SUMS-TRAILER.txt').write_text(''.join(f"{r['sha256']}  {n}\n" for n,r in reports.items()),encoding='ascii')
 print('Final verification and contact sheet complete',flush=True)
if __name__=='__main__':main()
