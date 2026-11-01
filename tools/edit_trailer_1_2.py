"""Compose the 1.2 trailer from real captures and editorial motion graphics."""
from pathlib import Path
import sys,os,math,json,subprocess,argparse
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'.tools/python'))
import cv2
import numpy as np
from PIL import Image,ImageDraw,ImageFont
cv2.setNumThreads(4)
os.environ['OPENCV_FFMPEG_CAPTURE_OPTIONS']='threads;2'
WORK=ROOT/'release-work/trailer-1.2';SRC=WORK/'sources'
OUT=ROOT/'releases/1.2.0/media';OUT.mkdir(parents=True,exist_ok=True)
FPS=30;DURATION=75
FONT=ROOT/'Content/UI/Fonts/Roboto-BoldCondensedItalic.ttf'
BODY=ROOT/'Content/UI/Fonts/Roboto-Medium.ttf'
YELLOW=(45,239,255);WHITE=(236,242,247);RED=(47,77,255)
def clamp(x):return min(1.,max(0.,x))
def ease(x):x=clamp(x);return 1-(1-x)**3
def smooth(x):x=clamp(x);return x*x*(3-2*x)

class Reader:
 def __init__(self,name):
  self.name=name;self.cap=cv2.VideoCapture(str(SRC/(name+'.mp4')))
  if not self.cap.isOpened():raise FileNotFoundError(name)
  self.fps=self.cap.get(cv2.CAP_PROP_FPS);self.count=int(self.cap.get(cv2.CAP_PROP_FRAME_COUNT));self.index=-1;self.image=None
 def frame(self,t):
  target=max(0,min(self.count-1,int(t*self.fps)))
  if target==self.index and self.image is not None:return self.image
  if target<self.index or target-self.index>20:self.cap.set(cv2.CAP_PROP_POS_FRAMES,target);self.index=target-1
  while self.index<target:
   ok,b=self.cap.read()
   if not ok:raise RuntimeError((self.name,target,self.index))
   self.index+=1;self.image=b
  return self.image

class Film:
 def __init__(self,width):
  self.w=width;self.h=width*9//16;self.s=width/3840;self.readers={};self.textures={}
  self.logo=cv2.imread(str(ROOT/'assets/source/branding/bmx-cruisers-logo.png'),cv2.IMREAD_UNCHANGED)
  self.intro=cv2.imread(str(WORK/'original-intro.png'))[19:-12,22:-15]
  rng=np.random.default_rng(1200);self.angles=rng.uniform(0,2*np.pi,190);self.speeds=rng.uniform(260,1750,190);self.life=rng.uniform(.3,1.5,190)
  self.chips=np.column_stack([rng.uniform(0,1,100),rng.uniform(0,1,100),rng.uniform(-1100,1800,100),rng.uniform(-1000,600,100),rng.uniform(7,32,100)])
 def reader(self,name):
  if name not in self.readers:self.readers[name]=Reader(name)
  return self.readers[name]
 def source(self,name,t):
  f=self.reader(name).frame(t)
  return cv2.resize(f,(self.w,self.h),interpolation=cv2.INTER_AREA) if f.shape[1]!=self.w else f.copy()
 def stamp(self,b,rgba,x,y,alpha=1.):
  x=int(x);y=int(y);hh,ww=rgba.shape[:2];x0=max(0,x);y0=max(0,y);x1=min(self.w,x+ww);y1=min(self.h,y+hh)
  if x1<=x0 or y1<=y0 or alpha<=0:return
  f=rgba[y0-y:y1-y,x0-x:x1-x];a=f[:,:,3:4].astype(np.float32)*(alpha/255)
  b[y0:y1,x0:x1]=(f[:,:,:3]*a+b[y0:y1,x0:x1]*(1-a)).astype(np.uint8)
 def text(self,b,string,x,y,size=96,color=WHITE,alpha=1,center=False,body=False):
  key=(string,int(size*self.s),color,body)
  if key not in self.textures:
   font=ImageFont.truetype(str(BODY if body else FONT),max(12,int(size*self.s)))
   bounds=font.getbbox(string);pad=max(4,int(8*self.s));ww=bounds[2]-bounds[0]+pad*2;hh=bounds[3]-bounds[1]+pad*2
   p=Image.new('RGBA',(ww,hh));d=ImageDraw.Draw(p)
   d.text((pad-bounds[0],pad-bounds[1]),string,font=font,fill=(*color[::-1],255),stroke_width=0)
   self.textures[key]=cv2.cvtColor(np.array(p),cv2.COLOR_RGBA2BGRA)
  tex=self.textures[key];xx=x*self.s-tex.shape[1]/2 if center else x*self.s
  self.stamp(b,tex,xx,y*self.s,alpha)
 def plate(self,b,title,small,local,length=99,garage=False):
  a=clamp(local/.20)*clamp((length-local)/.20);shift=(1-ease(local/.30))*-180
  x=int((145+shift)*self.s);y=int(1732*self.s);ww=int(3480*self.s);hh=int(280*self.s)
  if garage:x=int((1500+shift)*self.s);y=int(130*self.s);ww=int(2170*self.s);hh=int(270*self.s)
  layer=np.zeros((hh,ww,4),np.uint8)
  pts=np.array([[0,0],[ww,0],[ww-int(58*self.s),hh],[0,hh]],np.int32)
  cv2.fillConvexPoly(layer,pts,(30,23,17,234),cv2.LINE_AA)
  cv2.rectangle(layer,(0,0),(max(2,int(10*self.s)),hh),(*RED,255),-1)
  cv2.line(layer,(int(10*self.s),0),(ww,0),(*YELLOW,255),max(2,int(4*self.s)))
  self.stamp(b,layer,x,y,a)
  self.text(b,small,(1540 if garage else 185)+shift,160 if garage else 1760,38 if garage else 44,YELLOW,a,body=True)
  self.text(b,title,(1535 if garage else 180)+shift,235 if garage else 1831,82 if garage else 110,WHITE,a)
 def panel(self,b,img,x,y,w,h,alpha=1,nearest=False):
  x=int(x*self.s);y=int(y*self.s);w=int(w*self.s);h=int(h*self.s)
  f=cv2.resize(img,(w,h),interpolation=cv2.INTER_NEAREST if nearest else cv2.INTER_AREA)
  rgba=cv2.cvtColor(f,cv2.COLOR_BGR2BGRA);self.stamp(b,rgba,x,y,alpha)
  if 0<=x<self.w and 0<=y<self.h:cv2.rectangle(b,(x,y),(min(self.w-1,x+w),min(self.h-1,y+h)),(70,80,90),max(1,int(self.s*3)),cv2.LINE_AA)
 def original(self,t):
  b=np.full((self.h,self.w,3),(13,10,8),np.uint8)
  image=self.intro if t<1.6 else self.reader('original-c64').frame(1+t-1.6)
  self.panel(b,image,720,180,2400,1800,nearest=True)
  self.text(b,'COMMODORE 64  /  BMX RACERS',160,73,56,YELLOW,body=True)
  self.text(b,'REMEMBER THE RIDE?',1920,2010,68,WHITE,center=True)
  return b
 def transform(self,t):
  u=t-6;modern=self.source('course1',3.95);retro=self.reader('original-c64').frame(5.35)
  b=(modern*.23).astype(np.uint8)
  settle=ease(u/.72)
  oldx=720+(160-720)*settle;oldy=180+(380-180)*settle;oldw=2400+(1680-2400)*settle;oldh=oldw*.75
  newx=3840+(2000-3840)*settle;newy=538;neww=1680;newh=945
  expand=ease((u-2.35)/1.55)
  if expand>0:
   oldx-=2400*expand
   newx*=1-expand;newy*=1-expand;neww+=(3840-neww)*expand;newh+=(2160-newh)*expand
  self.panel(b,retro,oldx,oldy,oldw,oldh,1-expand,True)
  self.panel(b,modern,newx,newy,neww,newh)
  if u<3.9:
   self.text(b,'COMMODORE 64',1000,270,66,YELLOW,(1-expand)*settle,True)
   self.text(b,'UNREAL ENGINE 5',2840,425,66,YELLOW,(1-expand)*settle,True)
   for q,(px,py,vx,vy,size) in enumerate(self.chips):
    age=max(0,u-2.25-py*.35)
    if age<=0 or age>.8:continue
    xx=(oldx+oldw*.5+px*oldw*.5+vx*age)*self.s;yy=(oldy+py*oldh+vy*age+350*age*age)*self.s
    colors=[(24,191,70),(26,224,183),(214,50,35),(0,224,250)]
    col=np.array(colors[q%4])*(1-age/.8);r=max(2,int(size*self.s))
    cv2.rectangle(b,(int(xx),int(yy)),(int(xx)+r,int(yy)+r),tuple(map(int,col)),-1)
   if .1<expand<.99:
    seam=int(newx*self.s);cv2.line(b,(seam,0),(seam,self.h),YELLOW,max(2,int(11*self.s)))
   self.plate(b,'THE SAME COURSES. A NEW VIEW.','FROM THE COMMODORE 64 TO WINDOWS',u,4.1)
  else:
   b=self.source('course1',3.95+max(0,u-3.9))
   self.plate(b,'THE SAME COURSES. A NEW VIEW.','FROM THE COMMODORE 64 TO WINDOWS',u,6)
  return b
 def logo3d(self,b,local,final=False):
  impact=2/3;flight=ease(local/impact)
  if local<impact:scale=.035+1.13*flight
  else:
   age=local-impact;scale=(.80 if final else .91)+(.31 if final else .24)*math.exp(-age*6)*math.cos(age*14)
  opacity=clamp(local/.10)*(1 if final else clamp((4-local)/.4))
  width=3100*scale*self.s;height=width*self.logo.shape[0]/self.logo.shape[1]
  yaw=(1-flight)*math.radians(58);roll=(1-flight)*math.radians(-15);pitch=(1-flight)*math.radians(18)
  quad=np.array([[-.5,-.5,0],[.5,-.5,0],[.5,.5,0],[-.5,.5,0]],np.float64)*[width,height,1]
  ry=np.array([[math.cos(yaw),0,math.sin(yaw)],[0,1,0],[-math.sin(yaw),0,math.cos(yaw)]])
  rz=np.array([[math.cos(roll),-math.sin(roll),0],[math.sin(roll),math.cos(roll),0],[0,0,1]])
  rx=np.array([[1,0,0],[0,math.cos(pitch),-math.sin(pitch)],[0,math.sin(pitch),math.cos(pitch)]])
  quad=quad@(rz@ry@rx).T;focal=2200*self.s
  cx=self.w*.5+(1-flight)*900*self.s;cy=(870 if final else 1050)*self.s-(1-flight)*620*self.s
  points=np.column_stack([cx+quad[:,0]*focal/(focal+quad[:,2]),cy+quad[:,1]*focal/(focal+quad[:,2])]).astype(np.float32)
  x0=max(0,int(points[:,0].min())-5);y0=max(0,int(points[:,1].min())-5);x1=min(self.w,int(points[:,0].max())+5);y1=min(self.h,int(points[:,1].max())+5)
  if x1>x0 and y1>y0 and opacity>0:
   src=np.float32([[0,0],[self.logo.shape[1]-1,0],[self.logo.shape[1]-1,self.logo.shape[0]-1],[0,self.logo.shape[0]-1]])
   matrix=cv2.getPerspectiveTransform(src,(points-np.float32([x0,y0])).astype(np.float32))
   logo=cv2.warpPerspective(self.logo,matrix,(x1-x0,y1-y0),flags=cv2.INTER_LINEAR)
   side=logo.copy();side[:,:,:3]=(12,95,145)
   for depth in range(4,0,-1):self.stamp(b,side,x0+depth*(3+12*(1-flight))*self.s,y0+depth*3*self.s,opacity*.65)
   self.stamp(b,logo,x0,y0,opacity)
  age=local-impact
  if age>=0 and age<1.6:
   centre=np.array([self.w*.5,(950 if final else 1030)*self.s])
   count=110 if final else len(self.angles)
   for n in range(count):
    if age>self.life[n]:continue
    velocity=np.array([math.cos(self.angles[n]),math.sin(self.angles[n])])*self.s*self.speeds[n]
    pos=centre+velocity*age+np.array([0,360*self.s*age*age]);tail=pos-velocity*.027
    fade=(1-age/self.life[n]);col=np.array([YELLOW,WHITE,RED][n%3])*fade
    margin=max(3,int(8*self.s));xx0=max(0,int(min(pos[0],tail[0]))-margin);yy0=max(0,int(min(pos[1],tail[1]))-margin);xx1=min(self.w,int(max(pos[0],tail[0]))+margin);yy1=min(self.h,int(max(pos[1],tail[1]))+margin)
    if xx1>xx0 and yy1>yy0:
     roi=b[yy0:yy1,xx0:xx1];spark=np.zeros_like(roi)
     cv2.line(spark,tuple((tail-[xx0,yy0]).astype(int)),tuple((pos-[xx0,yy0]).astype(int)),tuple(map(int,col)),max(1,int((2+n%4)*self.s)),cv2.LINE_AA);cv2.add(roi,spark,dst=roi)
   radius=int((60+age*1800)*self.s)
   if age<.45:
    ring=np.zeros_like(b);cv2.circle(ring,tuple(centre.astype(int)),radius,tuple(int(c*(1-age/.45)) for c in YELLOW),max(1,int((5-age*8)*self.s)),cv2.LINE_AA);cv2.add(b,ring,dst=b)
  if 0<=age<.0667:b[:]=cv2.addWeighted(b,.36,np.full_like(b,255),.64,0)
  if 0<=age<.4:
   amp=math.exp(-age*12)*23*self.s;dx=amp*math.sin(age*110);dy=amp*math.cos(age*87)
   b[:]=cv2.warpAffine(b,np.float32([[1,0,dx],[0,1,dy]]),(self.w,self.h),borderMode=cv2.BORDER_REFLECT_101)
 def frame(self,t):
  if t<6:return self.original(t)
  if t<12:return self.transform(t)
  if t<16:
   b=self.source('course1',5.2+(t-12)*.5);b=(b*.52).astype(np.uint8);self.logo3d(b,t-12)
   return b
  if t<21:
   n=min(4,int(t-16));starts=[.2,.3,1.7,1.8,2.0];b=self.source('course'+str(n+1),starts[n]+t-16-n)
   self.plate(b,'FIVE ORIGINAL COURSES',f'COURSE {n+1:02d} / THE RECOVERED ROUTES',t-16,5);return b
  if t<26:
   b=self.source('course1',1.5+t-21);self.plate(b,'ONE LIFE. MAKE IT COUNT.','FINISH ALL FIVE TO EARN YOUR HIGH SCORE',t-21,5);return b
  if t<28:
   b=self.source('practice-menu',.6+t-26);self.plate(b,'LEARN THE LINE.','PICK A COURSE. FIND YOUR PACE.',t-26,2);return b
  if t<33:
   retrytime=.7+t-28 if t<31 else 6.7+(t-31)*.60
   b=self.source('practice-retry',retrytime);self.plate(b,'PRACTICE ANY COURSE.','RETRY. REFINE. RIDE AGAIN.',t-28,5);return b
  if t<45:
   n=min(5,int((t-33)/2));names=['cruiser','scooter','inline','quad','chair','bmx'];labels=['CRUISER','SCOOTER','INLINE SKATES','QUAD SKATES','WHEELCHAIR','BMX']
   b=self.source('garage-'+names[n],.55+(t-33-n*2))
   self.plate(b,'YOUR RIDER. YOUR STYLE.' if n<3 else 'SIX WAYS TO ROLL.',labels[n]+' / COLORS, OUTFITS & SKIN TONES',t-33,12,True);return b
  if t<47.5:
   b=self.source('controls-camera',t-45);self.plate(b,'KEYBOARD OR CONTROLLER.','LEFT STICK / D-PAD / KEYBOARD',t-45,2.5);return b
  if t<49:
   b=self.source('pace-menu',.5+t-47.5);self.plate(b,'YOUR PACE.','FIVE PACE SETTINGS / NORMAL KEEPS THE ORIGINAL TIMING',t-47.5,1.5);return b
  if t<52:
   b=self.source('controls-camera',3.5+t-49);self.plate(b,'YOUR VIEW.','ADJUST CAMERA TILT & DISTANCE',t-49,3);return b
  if t<56:
   b=self.source('course1',6+(t-52)*.49);self.plate(b,'A CLASSIC CHALLENGE.','SMOOTH SHORELINES / RIPPLING WATER / A SUNNY PARK',t-52,4);return b
  if t<60:
   b=self.source('course4',.5+(t-56)*.85);self.plate(b,'BUILT WITH UNREAL ENGINE 5.','MOVING FOLIAGE / WILDLIFE / A NEW PERSPECTIVE',t-56,4);return b
  if t<62.4:
   b=self.source('course1',3.0+(t-60));self.plate(b,'BEAT THE COURSE.','READ THE GAP. HOLD YOUR LINE.',t-60,2.4);return b
  if t<64.5:
   b=self.source('finish-approach',t-62.4);self.plate(b,'REACH THE FINISH.','EVERY CLEAN FINISH DESERVES A CELEBRATION',t-62.4,2.1);return b
  if t<68:
   b=self.source('finish',.2+t-64.5);self.plate(b,'LIGHT UP THE FINISH.','VIVID COLORS. ENORMOUS CELEBRATIONS.',t-64.5,3.5);return b
  b=self.source('finish',3+(t-68)*.5);small=cv2.resize(b,(320,180));small=cv2.GaussianBlur(small,(0,0),5);b=(cv2.resize(small,(self.w,self.h))*.16).astype(np.uint8)
  for i in range(12):
   xx=int(i*self.w/11);cv2.line(b,(xx,self.h),(int(self.w*.5+(xx-self.w*.5)*.20),int(self.h*.50)),(24,25,43),max(1,int(self.s*2)))
  self.logo3d(b,t-68,True);alpha=ease((t-69)/.4)
  self.text(b,'THE CLASSIC RIDE. REBUILT.',1920,1515,72,WHITE,alpha,True)
  self.text(b,'PLAY ON WINDOWS  /  VERSION 1.2',1920,1630,54,YELLOW,alpha,True)
  self.text(b,'RETROCOMPUTERIST.COM',1920,1780,112,YELLOW,alpha,True)
  self.text(b,'Based on BMX Racers - David Darling & Richard Darling / Mastertronic.',1920,2015,34,WHITE,alpha,True,True)
  self.text(b,'Unofficial recreation. Original-game emulator capture and actual Windows gameplay.',1920,2070,30,WHITE,alpha,True,True)
  return b

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--width',type=int,default=1920);ap.add_argument('--proofs',action='store_true');ap.add_argument('--patch-practice',action='store_true');args=ap.parse_args();film=Film(args.width)
 if args.proofs:
  proof=WORK/'proofs';proof.mkdir(exist_ok=True)
  for t in [0.8,3.5,7.4,9.3,12.45,12.70,13.4,16.5,22.0,27.0,29.0,30.0,32.7,34.1,36.1,38.1,40.1,42.1,44.1,46,48,50,54,58,63,66,70.5]:
   cv2.imwrite(str(proof/f'{t:05.2f}.jpg'),film.frame(t),[cv2.IMWRITE_JPEG_QUALITY,94])
  print('Storyboard proofs rendered',flush=True);return
 suffix='4K' if args.width==3840 else '1080p-draft';file=WORK/f'trailer-picture-{suffix}.mp4'
 original=file
 cap=None
 if args.patch_practice:
  cap=cv2.VideoCapture(str(original));assert cap.isOpened()
  file=WORK/f'trailer-picture-{suffix}-revised.mp4'
 ffmpeg='C:/dev/ffmpeg/bin/ffmpeg.exe'
 command=[ffmpeg,'-hide_banner','-loglevel','warning','-y','-f','rawvideo','-pix_fmt','bgr24','-s',f'{film.w}x{film.h}','-r','30','-i','-','-vf','scale=iw:ih:in_range=pc:out_range=tv:out_color_matrix=bt709,format=yuv420p','-c:v','h264_nvenc','-preset','p6','-rc','vbr','-cq','18','-b:v','0','-spatial-aq','1','-temporal-aq','1','-color_primaries','bt709','-color_trc','bt709','-colorspace','bt709','-movflags','+faststart','-an',str(file)]
 p=subprocess.Popen(command,stdin=subprocess.PIPE)
 try:
  for i in range(DURATION*FPS):
   if cap is None:frame=film.frame(i/FPS)
   else:
    ok,frame=cap.read();assert ok
    if 31*FPS<=i<33*FPS:frame=film.frame(i/FPS)
   p.stdin.write(frame.tobytes())
   if i%150==0:print(f'{suffix}: {i/FPS:.0f}/{DURATION}s composed',flush=True)
  p.stdin.close();code=p.wait()
  if code:raise RuntimeError(code)
  if cap is not None:
   cap.release();original.rename(WORK/f'trailer-picture-{suffix}-before-practice-trim.mp4');file.replace(original);file=original
 except BaseException:
  if p.poll() is None:p.terminate()
  raise
 print('Picture complete:',file,flush=True)
if __name__=='__main__':main()
