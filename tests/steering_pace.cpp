#include "Core/OriginalSimulation.h"
#include "Core/RideSettings.h"
#include <fstream>
#include <vector>
#include <cassert>
#include <cmath>
#include <iostream>
static std::vector<uint8_t> Read(const char* Path){std::ifstream F(Path,std::ios::binary);return {std::istreambuf_iterator<char>(F),{}};}
int main(){
    auto RAM=Read("Content/Data/original-ram.bin"),IO=Read("Content/Data/original-io.bin"),NoContactRAM=RAM;
    // Empty only the eight rider sprite masks in this fixture to isolate
    // steering from obstacles. Production replays retain all collision masks.
    std::fill(NoContactRAM.begin()+0xd8*64,NoContactRAM.begin()+0xe0*64,0);
    auto Load=[&](BMX::OriginalSimulation& S,int Pace){assert(S.Load(NoContactRAM.data(),NoContactRAM.size(),IO.data(),IO.size()));S.SetSteeringScale(BMX::RideSettings::SteeringScale(Pace));S.Start();};
    auto Hold=[&](BMX::OriginalSimulation& S,int Pace,int Hz,double Seconds,bool Left,bool Right){S.SetInput(Left,Right,false,false);for(int I=0;I<int(Hz*Seconds+.5);++I)S.Advance(BMX::RideSettings::Speed(Pace)/Hz);};
    BMX::OriginalSimulation Reference;Load(Reference,2);Hold(Reference,2,60,1.2,false,true);int Target=Reference.SpriteX();std::cout<<"Normal target "<<Target<<" state "<<int(Reference.GetState())<<std::endl;assert(Target>256&&Target<321);
    for(int Pace=0;Pace<5;++Pace){
        int At60=0;
        for(int Hz:{30,60,120}){
            BMX::OriginalSimulation S;Load(S,Pace);Hold(S,Pace,Hz,1.2,false,true);
            std::cout<<"Pace "<<BMX::RideSettings::Speed(Pace)<<" at "<<Hz<<" Hz: x="<<S.SpriteX()<<" (Normal "<<Target<<")"<<std::endl;
            // Original movement is emitted in row-associated bursts; allow
            // one eight-pixel burst at the end of this fixed wall-time sample.
            assert(std::abs(S.SpriteX()-Target)<=8);if(Hz==60)At60=S.SpriteX();
            if(Hz==120)assert(S.SpriteX()==At60);
            Hold(S,Pace,Hz,2.5,false,true);assert(S.SpriteX()==321);
            Hold(S,Pace,Hz,4,true,false);assert(S.SpriteX()==24);
            Hold(S,Pace,Hz,.5,false,true);assert(S.SpriteX()>50&&S.SpriteX()<70);
            Hold(S,Pace,Hz,.1,false,false);int Stop=S.SpriteX();Hold(S,Pace,Hz,.5,false,false);assert(S.SpriteX()==Stop);
        }
        BMX::OriginalSimulation S;Load(S,Pace);S.Reset();S.Start();Hold(S,2,60,1.2,false,true);assert(S.SpriteX()==Target);
        assert(S.Load(RAM.data(),RAM.size(),IO.data(),IO.size()));S.SetSteeringScale(BMX::RideSettings::SteeringScale(Pace));S.Start();
        for(int I=0;I<4000&&S.GetState()==BMX::RunState::Riding;++I)S.Advance(BMX::RideSettings::Speed(Pace)/120);
        assert(S.GetState()==BMX::RunState::Crashed&&S.Score()==53);
    }
    // Explicitly configured Normal must remain identical at every observation.
    BMX::OriginalSimulation A,B;assert(A.Load(RAM.data(),RAM.size(),IO.data(),IO.size()));assert(B.Load(RAM.data(),RAM.size(),IO.data(),IO.size()));
    B.SetSteeringScale(BMX::RideSettings::SteeringScale(2));A.Start();B.Start();
    for(int I=0;I<700;++I){bool L=I%97<28,R=I%97>65;A.SetInput(L,R,I%61<10,false);B.SetInput(L,R,I%61<10,false);A.TickCycles(1700);B.TickCycles(1700);
        assert(A.Memory()==B.Memory()&&A.Cycles()==B.Cycles()&&A.SpriteX()==B.SpriteX()&&A.SpriteY()==B.SpriteY()&&A.GetState()==B.GetState());}
    std::cout<<"Steering pace, bounds, reversal, release, reset, collision and Normal parity: PASSED\n";
}
