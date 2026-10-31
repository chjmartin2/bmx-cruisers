// Offline verification: search for an input replay that finishes the unmodified
// Course One simulation with collisions enabled. Not used by the game itself.
#include "Core/OriginalSimulation.h"
#include "Core/RideSettings.h"
#include <fstream>
#include <vector>
#include <memory>
#include <algorithm>
#include <set>
#include <iostream>

static std::vector<uint8_t> Read(const char* path) {
    std::ifstream f(path,std::ios::binary);
    return {std::istreambuf_iterator<char>(f),{}};
}
struct Candidate {
    std::shared_ptr<BMX::OriginalSimulation> sim;
    std::vector<int> inputs;
    double cost=0;
};
int main(int argc,char** argv) {
    const int Pace=argc>1?std::clamp(std::atoi(argv[1]),0,4):2;
    const char* Output=argc>2?argv[2]:"reference/extracted/course1-completion-replay.json";
    const auto ram=Read("Content/Data/original-ram.bin"),io=Read("Content/Data/original-io.bin");
    auto initial=std::make_shared<BMX::OriginalSimulation>();
    if(!initial->Load(ram.data(),ram.size(),io.data(),io.size()))return 2;
    initial->SetSteeringScale(BMX::RideSettings::SteeringScale(Pace));initial->Start();
    constexpr uint64_t Step=98525;
    std::vector<Candidate> beam{{initial,{},0}};
    for(int step=0;step<600&&!beam.empty();++step) {
        std::vector<Candidate> next;
        for(const auto& parent:beam)for(int action:{0,-1,1}) {
            Candidate c{std::make_shared<BMX::OriginalSimulation>(*parent.sim),parent.inputs,parent.cost};
            c.inputs.push_back(action);
            c.sim->SetInput(action<0,action>0,false,false);
            c.sim->TickCycles(Step);
            if(c.sim->GetState()==BMX::RunState::Complete) {
                // Replay independently from the source bytes, retaining collision.
                BMX::OriginalSimulation verify;
                verify.Load(ram.data(),ram.size(),io.data(),io.size());verify.SetSteeringScale(BMX::RideSettings::SteeringScale(Pace));verify.Start();
                for(int input:c.inputs){verify.SetInput(input<0,input>0,false,false);verify.TickCycles(Step);}
                if(verify.GetState()!=BMX::RunState::Complete||verify.Score()!=210)return 3;
                std::ofstream out(Output);
                out<<"{\"cycles_per_input\":"<<Step<<",\"score\":"<<verify.Score()<<",\"collisions_enabled\":true,\"inputs\":[";
                for(size_t i=0;i<c.inputs.size();++i)out<<(i?",":"")<<c.inputs[i];
                out<<"]}\n";
                std::cout<<"Verified full course: score="<<verify.Score()<<", input steps="<<c.inputs.size()<<"\n";
                return 0;
            }
            if(c.sim->GetState()!=BMX::RunState::Riding)continue;
            c.cost+=std::abs(c.sim->SpriteX()-176)*.01+(action? .08:0);
            next.push_back(std::move(c));
        }
        std::sort(next.begin(),next.end(),[](const Candidate&a,const Candidate&b){return a.cost<b.cost;});
        beam.clear();std::set<int> buckets;
        for(auto& c:next)if(buckets.insert(c.sim->SpriteX()/3).second){beam.push_back(std::move(c));if(beam.size()==80)break;}
        if(step%25==0)std::cout<<"Step "<<step<<", surviving positions "<<beam.size()<<std::endl;
    }
    std::cerr<<"No completion replay found in this bounded search\n";return 1;
}
