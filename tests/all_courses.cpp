#include "Core/OriginalSimulation.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cassert>
#include <iostream>
static std::vector<uint8_t> Read(const std::string& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
int main(){
 const int scores[]={210,250,375,500,625},offsets[]={0,8,18,33,53},delays[]={64,56,48,40,32};
 for(int c=1;c<=5;++c){
  const std::string base="Content/Data/course"+std::to_string(c);
  auto ram=Read(c==1?"Content/Data/original-ram.bin":base+"-ram.bin"),io=Read(c==1?"Content/Data/original-io.bin":base+"-io.bin");
  assert(ram.size()==65536&&io.size()==4096);
  assert(ram[0x52fc]==offsets[c-1]&&ram[0x4555]==delays[c-1]);
  auto bytes=Read("reference/extracted/course"+std::to_string(c)+"-completion-replay.json");std::string json(bytes.begin(),bytes.end());
  assert(json.find("\"collisions_enabled\":true")!=std::string::npos);
  auto pos=json.find("\"cycles_per_input\":");assert(pos!=std::string::npos);int step=std::stoi(json.substr(pos+19));
  auto begin=json.find('[',json.find("\"inputs\"")),end=json.find(']',begin);assert(begin!=std::string::npos&&end!=std::string::npos);
  std::string inputs=json.substr(begin+1,end-begin-1);for(char& ch:inputs)if(ch==',')ch=' ';std::istringstream stream(inputs);
  BMX::OriginalSimulation sim;assert(sim.Load(ram.data(),ram.size(),io.data(),io.size()));assert(sim.Score()==0);sim.Start();
  for(int a;stream>>a;){sim.SetInput(a==-1||a==4||a==6,a==1||a==5||a==7,a==3||a==6||a==7,a==2||a==4||a==5);sim.TickCycles(step);assert(sim.GetState()!=BMX::RunState::Crashed&&sim.GetState()!=BMX::RunState::Invalid);}
  assert(sim.GetState()==BMX::RunState::Complete&&sim.Score()==scores[c-1]&&sim.Rows()==scores[c-1]);
  sim.Reset();assert(sim.GetState()==BMX::RunState::Ready&&sim.Score()==0&&sim.Rows()==0);
  std::cout<<"Course "<<c<<": native sequence, pace, collision-enabled finish and retry passed\n";
 }
}
