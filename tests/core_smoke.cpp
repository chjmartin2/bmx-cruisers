#include "Core/OriginalSimulation.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <cassert>
#include <sstream>
#include <string>
#include <cmath>
#include "Core/RideSettings.h"

static std::vector<uint8_t> Read(const char* path) {
    std::ifstream file(path,std::ios::binary);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(file),{});
}
int main() {
    auto ram=Read("reference/extracted/course1-initial-ram.bin");
    auto io=Read("reference/extracted/course1-initial-io.bin");
    BMX::OriginalSimulation sim;
    assert(sim.Load(ram.data(),ram.size(),io.data(),io.size()));
    assert(sim.SpriteX()==176 && sim.SpriteY()==180 && sim.Score()==0);
    auto timingBytes=Read("reference/extracted/scroll-timing.json");std::string timing(timingBytes.begin(),timingBytes.end());
    std::vector<uint64_t> viceTimes;size_t cursor=0;
    while((cursor=timing.find("\"clock\":",cursor))!=std::string::npos){cursor+=8;viceTimes.push_back(std::stoull(timing.substr(cursor)));}
    assert(viceTimes.size()==20);sim.Start();std::vector<uint64_t> nativeTimes;
    for(int row=1;row<=20;++row){while(sim.Rows()<row)sim.TickCycles(1);nativeTimes.push_back(sim.Cycles());}
    double referenceSeconds=double(viceTimes.back()-viceTimes.front())/BMX::OriginalSimulation::ClockHz;
    double nativeSeconds=double(nativeTimes.back()-nativeTimes.front())/BMX::OriginalSimulation::ClockHz;
    std::cout<<"C64 timing, rows 1 to 20: VICE="<<referenceSeconds<<"s native="<<nativeSeconds<<"s\n";
    assert(std::abs(nativeSeconds/referenceSeconds-1)<.02);
    assert(BMX::RideSettings::Speed(BMX::RideSettings::Normal)==1.0);
    sim.Reset();
    sim.Start();
    for (int i=0;i<3000 && sim.GetState()==BMX::RunState::Riding;++i) sim.Advance(1.0/120);
    std::cout<<"Neutral: score="<<sim.Score()<<" rows="<<sim.Rows()<<" state="<<int(sim.GetState())<<" PC="<<std::hex<<sim.PC()<<std::dec<<" cycles="<<sim.Cycles()<<std::endl;
    assert(sim.GetState()==BMX::RunState::Crashed);
    assert(sim.Score()==53); // Clean VICE restart: unattended rider hits the shrub at score 53.
    sim.Reset(); sim.Start(); sim.SetInput(false,true,false,false);
    for(int i=0;i<60;++i) sim.Advance(1.0/120);
    assert(sim.SpriteX()>176);
    int x=sim.SpriteX();
    sim.Reset(); sim.Start(); sim.SetInput(false,true,false,false);
    for(int i=0;i<30;++i) sim.Advance(1.0/60);
    assert(sim.SpriteX()==x);
    std::cout<<"Input and render-rate independence passed; x="<<x<<"\n";
    // Compare two bounded row-boundary observations with the captured VICE
    // fixture. Only input polling is bypassed; the original movement code runs.
    auto steeringRAM=ram;
    steeringRAM[0xc720]=steeringRAM[0xc721]=steeringRAM[0xc722]=0xea;
    steeringRAM[0xc1ec]=0;steeringRAM[0xc1ed]=1;steeringRAM[0xc1ee]=0;
    assert(sim.Load(steeringRAM.data(),steeringRAM.size(),io.data(),io.size()));sim.Start();
    for(int row=1;row<=2;++row){
        while(sim.Rows()<row&&sim.GetState()==BMX::RunState::Riding)sim.TickCycles(1);
        assert(sim.Rows()==row&&sim.SpriteX()==176+row*8);
    }
    std::cout<<"VICE right-steering row boundaries matched: 184, 192\n";
    // A no-collision fixture isolates the original finish routine. The shipped
    // data is unmodified; this is not a successful obstacle-avoidance replay.
    ram[0x2d67]=0;
    assert(sim.Load(ram.data(),ram.size(),io.data(),io.size()));sim.Start();
    for(int i=0;i<6000&&sim.GetState()==BMX::RunState::Riding;++i)sim.Advance(1.0/120);
    std::cout<<"Finish: score="<<sim.Score()<<" rows="<<sim.Rows()<<" state="<<int(sim.GetState())<<std::endl;
    assert(sim.GetState()==BMX::RunState::Complete && sim.Score()==210);
    // Independently replay the recorded successful route with collisions enabled.
    auto original=Read("Content/Data/original-ram.bin");
    assert(sim.Load(original.data(),original.size(),io.data(),io.size()));sim.Start();
    auto replayBytes=Read("reference/extracted/course1-completion-replay.json");
    std::string replay(replayBytes.begin(),replayBytes.end());
    auto inputsStart=replay.find("\"inputs\":[");assert(inputsStart!=std::string::npos);
    auto cyclesStart=replay.find("\"cycles_per_input\":");assert(cyclesStart!=std::string::npos);
    uint64_t cyclesPerInput=std::stoull(replay.substr(cyclesStart+19));
    std::istringstream inputs(replay.substr(inputsStart+10));
    int input;char separator;int count=0;
    while(inputs>>input){
        assert(input>=-1&&input<=1);
        sim.SetInput(input<0,input>0,false,false);sim.TickCycles(cyclesPerInput);++count;
        if(!(inputs>>separator)||separator==']')break;
        assert(separator==',');
    }
    assert(count==213&&sim.GetState()==BMX::RunState::Complete&&sim.Score()==210);
    std::cout<<"Unmodified collision-enabled replay completed: 213 inputs, score 210\n";
    uint64_t finishCycles=sim.Cycles();
    for(int difficulty=0;difficulty<BMX::RideSettings::Count;++difficulty){
        assert(sim.Load(original.data(),original.size(),io.data(),io.size()));sim.Start();
        std::istringstream replayInputs(replay.substr(inputsStart+10));int command;double wall=0;
        while(replayInputs>>command){sim.SetInput(command<0,command>0,false,false);sim.TickCycles(cyclesPerInput);wall+=double(cyclesPerInput)/BMX::OriginalSimulation::ClockHz/BMX::RideSettings::Speed(difficulty);if(!(replayInputs>>separator)||separator==']')break;}
        assert(sim.Cycles()==finishCycles&&sim.Score()==210&&sim.GetState()==BMX::RunState::Complete);
        std::cout<<"Raw core (no steering compensation), playback clock "<<BMX::RideSettings::Speed(difficulty)<<"x: unchanged route, wall budget "<<wall<<"s\n";
    }
    return 0;
}
