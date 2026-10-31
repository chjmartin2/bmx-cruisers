import socket,re,json,time,sys
from pathlib import Path
s=socket.create_connection(('127.0.0.1',6514));s.settimeout(.35)
def read():
 b=b''
 while True:
  try:
   p=s.recv(65536)
   if not p:break
   b+=p
  except socket.timeout:break
 return b.decode('latin1')
def c(x):s.sendall((x+'\n').encode());return read()
read();time.sleep(.3);read()
p=str(Path('reference/extracted/course1-initial.vsf').resolve()).replace('\\','/')
trans=[]
for x in [f'undump "{p}"','delete','break $5300','g $ca00']:
 trans.append(dict(command=x,result=c(x)))
if '--ntsc' in sys.argv:trans.append(dict(command='resourceset "MachineVideoStandard" "2"',result=c('resourceset "MachineVideoStandard" "2"')))
rows=[]
for i in range(1,31):
 out=c('r');trans.append(dict(command='r',result=out))
 m=re.search(r'\.;5300\s+.*?\s(\d+)\s*(?:\n|$)',out)
 if m:rows.append(dict(row=i,clock=int(m.group(1))))
 if i<30:trans.append(dict(command='x',result=c('x')))
Path('reference/extracted/timing'+('-ntsc' if '--ntsc' in sys.argv else '-fresh')+'-transcript.json').write_text(json.dumps(trans,indent=2))
Path('reference/extracted/timing'+('-ntsc' if '--ntsc' in sys.argv else '-fresh')+'.json').write_text(json.dumps(rows,indent=2))
print(rows)
if len(rows)>1:print('ROW seconds',(rows[-1]['clock']-rows[0]['clock'])/(len(rows)-1)/985248)
c('quit');s.close()

