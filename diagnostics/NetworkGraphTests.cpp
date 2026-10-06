#include "../Main/NetworkGraph.h"
#include <cstdio>
#include <stdexcept>
using namespace S2Net;
static void Require(bool value,const char* message) {if(!value) throw std::runtime_error(message);}
static Bytes Delta(std::uint64_t base,std::uint32_t id,Bytes body,bool remove=false) {
  Bytes b;Put64(b,base);Put64(b,base+1);Put32(b,4);Put32(b,1);
  Put32(b,remove?0:1);
  if(!remove){Put32(b,id);Put32(b,0x12345678);Put32(b,1);Put32(b,static_cast<std::uint32_t>(body.size()));b.insert(b.end(),body.begin(),body.end());}
  Put32(b,remove?1:0);if(remove)Put32(b,id);return b;
}
int main(){try{
  NetworkGraph replica,host;
  auto initial=Delta(0,1,Bytes(2000,7)); replica.Apply(initial,true);
  auto captured=host.Capture(replica.Structure(),true);
  NetworkGraph other;other.Apply(captured,true);
  Require(other.Structure()==replica.Structure(),"record codec round trip");
  auto unchanged=host.Capture(replica.Structure(),false);
  Require(unchanged.size()<40,"unchanged bodies transmitted");other.Apply(unchanged,false);
  replica.Apply(Delta(1,1,{1,2,3}),false);
  other.Apply(host.Capture(replica.Structure(),false),false);
  Require(other.Structure()==replica.Structure(),"updated body mismatch");
  auto checkpoint=other.Structure();auto revision=other.Revision();
  for(auto malformed:std::vector<Bytes>{Delta(0,1,{3}),Bytes{1},Delta(revision,0,{3}),Delta(revision,99,{},true)}){
    bool rejected=false;try{other.Apply(malformed,false);}catch(const std::exception&){rejected=true;}
    Require(rejected && other.Structure()==checkpoint && other.Revision()==revision,"invalid update changed graph");
  }
  replica.Apply(Delta(2,1,{},true),false);
  other.Apply(host.Capture(replica.Structure(),false),false);
  Require(other.Objects()==0,"deletion failed");
  std::puts("Network graph: initial, changed records, sparse updates, deletions, atomic rejection passed");return 0;
}catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 1;}}
