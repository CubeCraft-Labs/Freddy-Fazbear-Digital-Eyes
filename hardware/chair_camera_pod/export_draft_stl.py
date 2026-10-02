"""Grid-export prototype STLs for chair_camera_pod.scad (0.4 mm pitch)."""

from pathlib import Path
import struct

import numpy as np

HERE = Path(__file__).resolve().parent
STEP = 0.4


def grid(lo, hi):
    axes = [np.arange(a,b,STEP,dtype=np.float32)+STEP/2 for a,b in zip(lo,hi)]
    return np.meshgrid(*axes,indexing="ij",sparse=True)


def box(X,Y,Z,x0,x1,y0,y1,z0,z1):
    return ((X>=x0)&(X<x1)&(Y>=y0)&(Y<y1)&(Z>=z0)&(Z<z1))


def disk(X,Y,Z,x,y,r,z0,z1):
    return ((X-x)**2+(Y-y)**2<=r*r)&(Z>=z0)&(Z<z1)


def xslot(X,Y,Z,a,b,y,r,z0,z1):
    return (box(X,Y,Z,min(a,b),max(a,b),y-r,y+r,z0,z1)
            |disk(X,Y,Z,a,y,r,z0,z1)|disk(X,Y,Z,b,y,r,z0,z1))


def yslot(X,Y,Z,x,a,b,r,z0,z1):
    return (box(X,Y,Z,x-r,x+r,min(a,b),max(a,b),z0,z1)
            |disk(X,Y,Z,x,a,r,z0,z1)|disk(X,Y,Z,x,b,r,z0,z1))


def tray():
    lo,hi=(-39,-27,0),(39,27,14)
    X,Y,Z=grid(lo,hi)
    solid=box(X,Y,Z,-27,27,-27,27,0,3)
    solid|=box(X,Y,Z,-27,-24,-27,27,3,13)
    solid|=box(X,Y,Z,24,27,-27,27,3,13)
    solid|=box(X,Y,Z,-27,27,24,27,3,13)
    solid|=box(X,Y,Z,-39,-26,-18,18,0,4)
    solid|=box(X,Y,Z,26,39,-18,18,0,4)
    for x in (-17,17):
        for y in (-17,17): solid|=disk(X,Y,Z,x,y,3.5,3,6)
    for x in (-17,17):
        for y in (-17,17): solid&=~xslot(X,Y,Z,x-2,x+2,y,1.5,-1,9)
    for x in (-33,33): solid&=~yslot(X,Y,Z,x,-11,11,2.5,-1,6)
    for x in (-25.5,25.5):
        for y in (-23,23): solid&=~disk(X,Y,Z,x,y,0.8,5,14)
    return solid,lo


def cover():
    lo,hi=(-27,-27,0),(27,27,2.4)
    X,Y,Z=grid(lo,hi)
    solid=box(X,Y,Z,-27,27,-27,27,0,2.4)
    solid&=~disk(X,Y,Z,0,6,9,-1,4)
    for x in (-15.5,15.5):solid&=~yslot(X,Y,Z,x,0,11,3.5,-1,4)
    for x in (-25.5,25.5):
        for y in (-23,23):solid&=~disk(X,Y,Z,x,y,1.2,-1,4)
    return solid,lo


def write_stl(path,solid,lo):
    exposed=[]
    for axis in range(3):
        for sign in (-1,1):
            neighbor=np.roll(solid,-sign,axis=axis)
            face=solid&~neighbor
            edge=[slice(None)]*3;edge[axis]=0 if sign==-1 else -1
            face[tuple(edge)]=solid[tuple(edge)]
            exposed.append((axis,sign,np.argwhere(face)))
    facets=sum(2*len(v) for _,_,v in exposed)
    with path.open("wb") as out:
        out.write(b"Freddy concealed chair pod prototype".ljust(80,b"\0"))
        out.write(struct.pack("<I",facets))
        for axis,sign,points in exposed:
            normal=[0.,0.,0.];normal[axis]=float(sign)
            for i,j,k in points:
                x0,y0,z0=(lo[0]+i*STEP,lo[1]+j*STEP,lo[2]+k*STEP)
                x1,y1,z1=x0+STEP,y0+STEP,z0+STEP
                if axis==0:
                    x=x1 if sign==1 else x0
                    quad=[(x,y0,z0),(x,y1,z0),(x,y1,z1),(x,y0,z1)]
                elif axis==1:
                    y=y1 if sign==1 else y0
                    quad=[(x0,y,z0),(x0,y,z1),(x1,y,z1),(x1,y,z0)]
                else:
                    z=z1 if sign==1 else z0
                    quad=[(x0,y0,z),(x1,y0,z),(x1,y1,z),(x0,y1,z)]
                if sign==-1:quad.reverse()
                for a,b,c in ((0,1,2),(0,2,3)):
                    out.write(struct.pack("<12fH",*normal,*quad[a],*quad[b],*quad[c],0))
    print(path.name,facets,"facets",round(path.stat().st_size/1e6,1),"MB")


if __name__=="__main__":
    for name,build in (("chair_camera_tray_draft.stl",tray),
                       ("chair_camera_cover_draft.stl",cover)):
        solid,lo=build();write_stl(HERE/name,solid,lo)
