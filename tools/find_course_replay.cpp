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
#include <string>

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
    const int Course=argc>3?std::clamp(std::atoi(argv[3]),1,5):1;
    const std::string Base="Content/Data/course"+std::to_string(Course);
    const auto ram=Read(Course==1?"Content/Data/original-ram.bin":(Base+"-ram.bin").c_str()),io=Read(Course==1?"Content/Data/original-io.bin":(Base+"-io.bin").c_str());
    auto initial=std::make_shared<BMX::OriginalSimulation>();
    if(!initial->Load(ram.data(),ram.size(),io.data(),io.size()))return 2;
    initial->SetSteeringScale(BMX::RideSettings::SteeringScale(Pace));initial->Start();
    const uint64_t Step=argc>4?std::max(1,std::atoi(argv[4])):98525;
    std::vector<int> prefix;
    const int StartY=argc>5?std::clamp(std::atoi(argv[5]),60,240):180;
    for(int I=0;I<200&&initial->SpriteY()!=StartY;++I){int action=initial->SpriteY()<StartY?2:3;initial->SetInput(false,false,action==3,action==2);initial->TickCycles(Step);prefix.push_back(action);if(initial->GetState()!=BMX::RunState::Riding)return 4;}
    std::vector<Candidate> beam{{initial,prefix,0}};
    const bool Vertical=argc>6&&std::atoi(argv[6])!=0;
    const bool Adaptive=argc>7&&std::atoi(argv[7])!=0;
    const std::vector<int> Actions=Vertical?std::vector<int>{0,-1,1,2,3,4,5,6,7}:std::vector<int>{0,-1,1};
    auto Input=[](BMX::OriginalSimulation& Sim,int A){Sim.SetInput(A==-1||A==4||A==6,A==1||A==5||A==7,A==3||A==6||A==7,A==2||A==4||A==5);};
    for(int step=0;step<3000&&!beam.empty();++step) {
        std::vector<Candidate> next;
        for(const auto& parent:beam)for(int action:(Adaptive&&((Course==4&&parent.sim->Rows()>=245&&parent.sim->Rows()<=350)||(Course==5&&parent.sim->Rows()>=450))?std::vector<int>{0,-1,1,2,3,4,5,6,7}:Actions)) {
            Candidate c{std::make_shared<BMX::OriginalSimulation>(*parent.sim),parent.inputs,parent.cost};
            c.inputs.push_back(action);
            Input(*c.sim,action);
            c.sim->TickCycles(Step);
            if(c.sim->GetState()==BMX::RunState::Complete) {
                // Replay independently from the source bytes, retaining collision.
                BMX::OriginalSimulation verify;
                verify.Load(ram.data(),ram.size(),io.data(),io.size());verify.SetSteeringScale(BMX::RideSettings::SteeringScale(Pace));verify.Start();
                for(int input:c.inputs){Input(verify,input);verify.TickCycles(Step);}
                if(verify.GetState()!=BMX::RunState::Complete||verify.Score()!=verify.Rows())return 3;
                std::ofstream out(Output);
                out<<"{\"course\":"<<Course<<",\"cycles_per_input\":"<<Step<<",\"score\":"<<verify.Score()<<",\"collisions_enabled\":true,\"inputs\":[";
                for(size_t i=0;i<c.inputs.size();++i)out<<(i?",":"")<<c.inputs[i];
                out<<"]}\n";
                std::cout<<"Verified full course: score="<<verify.Score()<<", input steps="<<c.inputs.size()<<"\n";
                return 0;
            }
            if(c.sim->GetState()!=BMX::RunState::Riding)continue;
            c.cost+=(action? .00001:0);
            next.push_back(std::move(c));
        }
        std::sort(next.begin(),next.end(),[](const Candidate&a,const Candidate&b){return a.cost+std::abs(a.sim->SpriteX()-176)*.0001<b.cost+std::abs(b.sim->SpriteX()-176)*.0001;});
        beam.clear();std::set<int> buckets;
        for(auto& c:next)if(buckets.insert(Adaptive?(c.sim->SpriteY()/(Course==5&&c.sim->Rows()>=540?4:10))*512+c.sim->SpriteX()/((Course==5&&c.sim->Rows()>=540)?2:(Course==4&&c.sim->Rows()>=245)||(Course==5&&c.sim->Rows()>=450)?3:1):((c.sim->SpriteY()/2)*512+c.sim->SpriteX()/2)*128+(c.sim->SpriteFrame()-0xd8)*8+c.inputs.back()+1).second){int Limit=Adaptive?(Course==5&&c.sim->Rows()>=540?600:300):Vertical?1600:1200;beam.push_back(std::move(c));if(beam.size()==Limit)break;}
        if(step%25==0)std::cout<<"Step "<<step<<", surviving positions "<<beam.size()<<", row "<<(beam.empty()?0:beam.front().sim->Rows())<<", y "<<(beam.empty()?0:beam.front().sim->SpriteY())<<std::endl;
    }
    std::cerr<<"No completion replay found in this bounded search\n";return 1;
}
