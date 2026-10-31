import socket,time,re,json
from pathlib import Path
s=socket.create_connection(('127.0.0.1',6514));s.settimeout(.2)
def read():
 b=b''
 while True:
  try:b+=s.recv(65536)
  except socket.timeout:break
 return b.decode('latin1')
def c(x):
 s.sendall((x+'\n').encode());return read()
read()
p=str(Path('reference/extracted/course1-initial.vsf').resolve()).replace('\\','/')
for x in [f'undump "{p}"','delete','break $5300','r','g $ca00','r']:
 print(x,repr(c(x)),flush=True)
s.close()
