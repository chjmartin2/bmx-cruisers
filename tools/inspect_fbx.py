"""Small read-only FBX binary inspector for source asset transforms and rig names."""
from pathlib import Path
import struct,zlib,sys

def read(path):
    data=Path(path).read_bytes();version=struct.unpack_from('<I',data,23)[0]
    wide=version>=7500;header=25 if wide else 13
    def prop(p):
        kind=chr(data[p]);p+=1
        formats={'Y':'h','C':'?','I':'i','F':'f','D':'d','L':'q'}
        if kind in formats:
            fmt=formats[kind];return struct.unpack_from('<'+fmt,data,p)[0],p+struct.calcsize(fmt)
        if kind in 'SR':
            n=struct.unpack_from('<I',data,p)[0];p+=4;return data[p:p+n],p+n
        count,encoding,length=struct.unpack_from('<III',data,p);p+=12
        raw=zlib.decompress(data[p:p+length]) if encoding else data[p:p+length]
        fmt={'f':'f','d':'d','i':'i','l':'q','b':'?','c':'b'}[kind]
        return struct.unpack('<'+str(count)+fmt,raw),p+length
    def node(p):
        end,n,size,nl=struct.unpack_from('<QQQB' if wide else '<IIIB',data,p)
        if not end:return None,p+header
        start=p;p+=header;name=data[p:p+nl].decode();p+=nl;props=[]
        props_start=p
        for i in range(n):value,p=prop(p);props.append(value)
        raw_props=data[props_start:p];children=[]
        while p<end-header:
            child,p=node(p)
            if child:children.append(child)
        return dict(name=name,props=props,children=children,raw_props=raw_props),end
    nodes=[];p=27
    while p<len(data)-header:
        value,newp=node(p)
        if not value:break
        nodes.append(value);p=newp
    return nodes

if __name__=='__main__':
    for section in read(sys.argv[1]):
        if section['name']=='Objects':
            for node in section['children']:
                if node['name'] in ['Model','Geometry','Material']:print(node['name'],node['props'][:3])
