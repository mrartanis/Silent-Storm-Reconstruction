#include "../Main/NetworkTransport.h"
#include <chrono>
#include <cstdio>
#include <stdexcept>
#include <thread>
static void Check(bool b,int line) { if(!b) throw std::runtime_error("network transport assertion at line "+std::to_string(line)); }
#define Require(condition) Check((condition),__LINE__)
int main() {
 try {
  using namespace S2Net;
  Packet p{Message::Command,123,{1,2,3,4,5}}, q{Message::State,124,{6,7}};
  auto bytes=Encode(p), extra=Encode(q); bytes.insert(bytes.end(),extra.begin(),extra.end());
  Decoder d; std::vector<Packet> got;
  for(auto b:bytes) { auto one=d.Feed(&b,1); got.insert(got.end(),one.begin(),one.end()); }
  Require(got.size()==2 && got[0].data==p.data && got[1].sequence==124);
  bool failed=false; bytes[4]=99; try { Decoder bad; bad.Feed(bytes.data(),bytes.size()); } catch(...) { failed=true; } Require(failed);
  Transport host,client; host.Listen(0); Require(host.Status()==Connection::Listening && host.Port()!=0);
  Transport occupied; occupied.Listen(host.Port()); Require(occupied.Status()==Connection::Failed);
  Require(occupied.Diagnostics().systemError!=0 && occupied.Diagnostics().operation=="listen");
  client.Connect("127.0.0.1",host.Port());
  auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
  while(host.Status()!=Connection::Connected || client.Status()!=Connection::Connected) { host.Poll(); client.Poll(); Require(std::chrono::steady_clock::now()<deadline); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
  p.data.resize(2*1024*1024,42); client.Send(p); host.Send(q);
  bool received=false,reply=false;
  while(!received || !reply) {
    for(auto& v:host.Poll()) { Require(v.data==p.data && v.sequence==p.sequence); received=true; }
    for(auto& v:client.Poll()) { Require(v.data==q.data); reply=true; }
    Require(std::chrono::steady_clock::now()<deadline); std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  client.Close(); while(host.Status()!=Connection::Failed) { host.Poll(); Require(std::chrono::steady_clock::now()<deadline); }
  Require(host.Diagnostics().operation=="recv-eof");
  host.Listen(0); Require(host.Status()==Connection::Listening); host.Close();
  std::puts("TCP fragmented/coalesced packets, localhost exchange, occupied port, disconnect, restart passed"); return 0;
 } catch(const std::exception& e) { std::fprintf(stderr,"%s\n",e.what()); return 1; }
}
