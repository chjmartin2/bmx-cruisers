#define CHIPS_IMPL
#include "OriginalSimulation.h"
#include <algorithm>
#include <cstring>
#include <cmath>

namespace BMX {
bool OriginalSimulation::Load(const uint8_t* ram, size_t size, const uint8_t* io, size_t ioSize) {
    if (size != Source.size() || ioSize != SourceIO.size() || !ram || !io) return false;
    std::memcpy(Source.data(), ram, size);
    std::memcpy(SourceIO.data(), io, ioSize);
    if (Source[0xca00] != 0x20 || Source[0x5700] != 3 || Source[0x5707] != 7) return false;
    Loaded = true;
    Reset();
    return true;
}

void OriginalSimulation::Prefetch(uint16_t address) {
    Pins = M6502_SYNC | M6502_RW;
    M6502_SET_ADDR(Pins, address);
    M6502_SET_DATA(Pins, RAM[address]);
    m6502_set_pc(&Cpu, address);
}

void OriginalSimulation::Reset() {
    if (!Loaded) return;
    RAM = Source; IO = SourceIO;
    m6502_desc_t desc{};
    Pins = m6502_init(&Cpu, &desc);
    m6502_set_a(&Cpu, 1); m6502_set_x(&Cpu, 5); m6502_set_y(&Cpu, 0);
    m6502_set_s(&Cpu, 0xf8); m6502_set_p(&Cpu, 0x21);
    // A small replacement for the ROM IRQ wrapper. No Commodore ROM is shipped.
    const uint8_t irq[] = {0x48,0x8a,0x48,0x98,0x48,0x6c,0x14,0x03};
    std::memcpy(RAM.data()+0xf000, irq, sizeof irq);
    // Restore Y, X, A and return from interrupt after the original tune routine.
    const uint8_t ret[] = {0x68,0xa8,0x68,0xaa,0x68,0x40};
    std::memcpy(RAM.data()+0xf020, ret, sizeof ret);
    RAM[0xfffe]=0; RAM[0xffff]=0xf0;
    RAM[0xc5]=0x40; RAM[0x28d]=0;
    Ticks=0; NextIRQ=16421; FractionalCycles=0; ScrollRows=0;LastScrollTick=0;
    Collision=0; Joystick=0xff; IRQActive=false; ServiceDelay=0;
    SteeringScale=1;SteeringFraction=0;SteeringDirection=0;SteeringStepActive=false;
    Prefetch(0xca00);
    State=RunState::Ready;
}

void OriginalSimulation::SetInput(bool left, bool right, bool up, bool down) {
    uint8_t Previous=Joystick;
    Joystick=0xff;
    if (up) Joystick &= ~1;
    if (down) Joystick &= ~2;
    if (left) Joystick &= ~4;
    if (right) Joystick &= ~8;
    if((Previous&12)!=(Joystick&12)){SteeringFraction=0;SteeringDirection=0;}
}

void OriginalSimulation::SetSteeringScale(double scale){
    SteeringScale=std::isfinite(scale)?std::clamp(scale,.5,2.):1.;
    SteeringFraction=0;SteeringDirection=0;
}

void OriginalSimulation::FinishSteeringStep(){
    int NativeX=IO[0]+((IO[0x10]&1)?256:0),Delta=NativeX-SteeringStartX;
    SteeringStepActive=false;
    auto SetX=[&](int X){IO[0]=uint8_t(X);IO[0x10]=uint8_t((IO[0x10]&~1)|(X>=256?1:0));};
    SetX(SteeringStartX);
    if(Delta==0){SteeringFraction=0;return;}
    if(Delta!=SteeringDirection){SteeringFraction=0;SteeringDirection=Delta;}
    SteeringFraction+=SteeringScale;
    int Steps=int(SteeringFraction+1e-9);SteeringFraction-=Steps;
    for(int I=0;I<Steps;++I){
        int X=SpriteX(),Next=std::clamp(X+Delta,24,321);
        if(Next==X){SteeringFraction=0;break;}
        SetX(Next);
        // Sweep each compensated pixel through the original sprite/background
        // collision masks; added steps cannot jump through a narrow obstacle.
        for(int Row=0;Row<21;++Row)CheckRasterCollision(SpriteY()+Row);
        if(Collision&1)break;
    }
}

int OriginalSimulation::Score() const {
    int value=0;
    for (int i=0x40a; i<0x410; ++i) value=value*10+std::clamp(int(RAM[i])-0xb0,0,9);
    return value;
}

uint8_t OriginalSimulation::Read(uint16_t a) {
    if (a >= 0xd000 && a < 0xd400) {
        int r=a&0x3f;
        int line=int(((Ticks+7112)/63)%312);
        if (r==0x12) return uint8_t(line);
        if (r==0x11) return (IO[r]&0x7f)|(line>=256?0x80:0);
        if (r==0x1f) { uint8_t result=Collision; Collision=0; return result; }
        if (r==0x1e) return 0;
        return IO[r];
    }
    if (a==0xdc00) return Joystick;
    if (a==0xdc01) return 0xff;
    if (a==0xdc0d) { IRQActive=false; return 0x81; }
    if (a>=0xd000 && a<0xe000) return IO[a-0xd000];
    return RAM[a];
}

void OriginalSimulation::Write(uint16_t a, uint8_t v) {
    if (a>=0xd000 && a<0xd400) { IO[a&0x3f]=v; return; }
    if (a>=0xd000 && a<0xe000) { IO[a-0xd000]=v; return; }
    RAM[a]=v;
}

bool OriginalSimulation::BackgroundSolid(int x, int y) const {
    if (x<0 || x>=320 || y<0 || y>=200) return false;
    int screen=(IO[0x18]>>4)*1024;
    int charset=((IO[0x18]>>1)&7)*2048;
    int cell=(y/8)*40+x/8;
    uint8_t glyph=RAM[charset+RAM[screen+cell]*8+y%8];
    uint8_t color=IO[0x800+cell]&15;
    if ((IO[0x16]&16) && (color&8)) return ((glyph>>(6-((x%8)/2)*2))&3)>=2;
    return (glyph&(128>>(x%8)))!=0;
}

void OriginalSimulation::CheckRasterCollision(int line) {
    if(Contacts){if(line==SpriteY()+10){for(double F:{0.,30.,60.})for(double S:{-15.,0.,15.})if(Contacts->Hit(ForwardPixels()*7.5+F,RightPixels()*7.5+S)){Collision|=1;return;}}return;}
    if(!OriginalCollision)return;
    if (!(IO[0x15]&1)) return;
    int row=line-SpriteY();
    const bool expandY=(IO[0x17]&1)!=0;
    if (expandY) row/=2;
    if (line<SpriteY() || row<0 || row>=21) return;
    const int sprite=RAM[0x7f8]*64;
    const bool multi=(IO[0x1c]&1)!=0, expandX=(IO[0x1d]&1)!=0;
    for (int x=0; x<24; ++x) {
        uint8_t byte=RAM[sprite+row*3+x/8];
        bool solid=multi ? ((byte>>(6-((x%8)/2)*2))&3)!=0 : (byte&(128>>(x%8)))!=0;
        if (!solid) continue;
        int px=SpriteX()-24+x*(expandX?2:1);
        if (BackgroundSolid(px,line-51) || (expandX && BackgroundSolid(px+1,line-51))) {
            Collision|=1; return;
        }
    }
}

void OriginalSimulation::Advance(double seconds) {
    if (State!=RunState::Riding || seconds<=0 || !std::isfinite(seconds)) return;
    FractionalCycles += seconds*ClockHz;
    auto cycles=uint64_t(FractionalCycles);
    FractionalCycles-=double(cycles);
    TickCycles(cycles);
}

void OriginalSimulation::TickCycles(uint64_t cycles) {
    if (State!=RunState::Riding) return;
    for (uint64_t i=0; i<cycles && State==RunState::Riding; ++i) {
        uint64_t raster=(Ticks+7112)%19656;
        int line=int(raster/63), column=int(raster%63);
        if (column==55) CheckRasterCollision(line);
        ++Ticks;
        if (Ticks>=NextIRQ) { NextIRQ+=16421; IRQActive=true; }
        if (IRQActive) Pins|=M6502_IRQ; else Pins&=~M6502_IRQ;
        if (ServiceDelay>0) { --ServiceDelay; continue; }
        // VIC bad-line fetches and the active rider's sprite DMA steal bus time.
        bool bad=line>=48 && line<=247 && (line&7)==(IO[0x11]&7) && column>=15 && column<55;
        bool sprite=(IO[0x15]&1) && line>=SpriteY() && line<SpriteY()+21 && column>=55 && column<58;
        if (bad || sprite) continue;
        if (Pins&M6502_SYNC) {
            uint16_t pc=M6502_GET_ADDR(Pins);
            if(SteeringScale!=1.){
                // These two call sites move the rider only (never other sprites).
                // Keep the native routine's cycles, bounds checks and animation.
                if(pc==0xc788||pc==0xc7b2){SteeringStartX=SpriteX();SteeringStepActive=true;}
                if(SteeringStepActive&&(pc==0xc78b||pc==0xc7b5))FinishSteeringStep();
            }
            if (pc==0xca80) { State=RunState::Crashed; break; }
            if (pc==0x5500) { State=RunState::Complete; break; }
            if (pc==0x5300) {++ScrollRows;LastScrollTick=Ticks;}
            if (pc==0xea31) {
                // Kernal clock/keyboard service; account for its scan overhead.
                uint32_t clock=(RAM[0xa0]<<16)|(RAM[0xa1]<<8)|RAM[0xa2]; ++clock;
                RAM[0xa0]=uint8_t(clock>>16); RAM[0xa1]=uint8_t(clock>>8); RAM[0xa2]=uint8_t(clock);
                IRQActive=false; Pins&=~M6502_IRQ;
                ServiceDelay=650;
                Prefetch(0xf020);
            }
            if (pc==0xffd2 || pc==0xffe4) { State=RunState::Invalid; break; }
        }
        Pins=m6502_tick(&Cpu,Pins);
        uint16_t a=M6502_GET_ADDR(Pins);
        if (Pins&M6502_RW) { M6502_SET_DATA(Pins,Read(a)); }
        else Write(a,M6502_GET_DATA(Pins));
    }
}
}
