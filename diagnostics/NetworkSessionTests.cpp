#include "../Main/NetworkSession.h"
#include <chrono>
#include <cstdio>
#include <stdexcept>
#include <thread>
using namespace S2Net;
static void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class Condition> void Pump(NetworkSession& host,NetworkSession& client,Condition done) {
  auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
  while(!done() && std::chrono::steady_clock::now()<deadline){host.Poll();client.Poll();std::this_thread::sleep_for(std::chrono::milliseconds(1));}
  Require(done(),"network session test timed out");
}
int main(){try{
  int commands=0,queries=0,clientInitial=0,hostTicks=0,events=0,results=0;
  SessionCallbacks server,mirror;
  server.createInitial=[] {return Bytes{10};};
  server.segment=[&]{++hostTicks;return Bytes{11};};
  server.command=[&](unsigned slot,const Bytes& bytes){Require(slot==1 && bytes==Bytes{42},"command request routing");++commands;return Bytes{12};};
  server.query=[&](unsigned slot,const Bytes& bytes){Require(slot==1 && bytes==Bytes{43},"preview request routing");++queries;return Bytes{13};};
  server.drainEvents=[] {return std::vector<Bytes>{Bytes{14}};};
  mirror.applyState=[&](const Bytes& data,bool initial){Require(data==Bytes{static_cast<std::uint8_t>(initial?10:11)},"client state payload");if(initial)++clientInitial;};
  mirror.event=[&](const Bytes& data){Require(data==Bytes{14},"event payload");++events;};
  mirror.result=[&](std::uint64_t id,const Bytes& data,bool query){Require(id>0 && data==Bytes{static_cast<std::uint8_t>(query?13:12)},"result correlation");++results;};
  NetworkSession host({"build","data"},server),client({"build","data"},mirror);
  host.Host(0);client.Connect("127.0.0.1",host.Port());
  Pump(host,client,[&]{return host.State()==SessionState::Playing && client.State()==SessionState::Playing;});
  Require(clientInitial==1 && host.LocalSlot()==0 && client.LocalSlot()==1,"initial state and player slots");
  Require(client.Submit({42})!=0 && client.Query({43})!=0,"request IDs");
  Pump(host,client,[&]{return results==2 && events>0;});
  Require(commands==1 && queries==1 && hostTicks>0,"commands or world ticks missing");
  // Preview requests and End can arrive in one receive batch followed by EOF.
  for(int i=0;i<10;++i)client.Query({43});
  client.Finish("Player left");
  Pump(host,client,[&]{return host.State()==SessionState::Finished || host.State()==SessionState::Failed;});
  Require(host.Status()=="Player left","termination reason lost");
  NetworkSession compatible({"build","data"},server),wrong({"build","different"},mirror);
  compatible.Host(0);wrong.Connect("127.0.0.1",compatible.Port());
  Pump(compatible,wrong,[&]{return compatible.State()==SessionState::Failed && wrong.State()==SessionState::Failed;});
  Require(wrong.Status()=="Different effective game data","compatibility failure is unclear");
  NetworkSession cancelled({"build","data"},server);cancelled.Host(0);cancelled.Finish("Cancelled");
  Require(cancelled.State()==SessionState::Finished,"cannot cancel waiting host");
  // A raw peer repeats a command ID to prove it executes exactly once.
  NetworkSession dedup({"build","data"},server);dedup.Host(0);
  Transport peer;peer.Connect("127.0.0.1",dedup.Port());
  Bytes hello;Put32(hello,ProtocolVersion);PutString(hello,"build");PutString(hello,"data");
  bool sentHello=false,sentReady=false;
  auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
  while(dedup.State()!=SessionState::Playing && std::chrono::steady_clock::now()<deadline){
    dedup.Poll();auto packets=peer.Poll();
    if(!sentHello && peer.Status()==Connection::Connected){peer.Send({Message::Hello,0,hello});sentHello=true;}
    for(auto packet:packets) if(packet.type==Message::Initial){peer.Send({Message::Ready,0,{}});sentReady=true;}
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  Require(sentReady && dedup.State()==SessionState::Playing,"raw-peer setup");
  int before=commands;peer.Send({Message::Command,1,{42}});peer.Send({Message::Command,1,{42}});
  for(int i=0;i<100;++i){peer.Poll();dedup.Poll();std::this_thread::sleep_for(std::chrono::milliseconds(1));}
  Require(commands==before+1,"duplicate command executed twice");
  peer.Close();dedup.Poll();
  Require(dedup.State()==SessionState::Failed,"disconnect did not stop match");
  NetworkSession wrongBuildHost({"build","data"},server),wrongBuild({"other","data"},mirror);
  wrongBuildHost.Host(0);wrongBuild.Connect("127.0.0.1",wrongBuildHost.Port());
  Pump(wrongBuildHost,wrongBuild,[&]{return wrongBuildHost.State()==SessionState::Failed && wrongBuild.State()==SessionState::Failed;});
  Require(wrongBuild.Status()=="Different game builds","build mismatch not reported");
  Transport rawHost;rawHost.Listen(0);
  NetworkSession rawClient({"build","data"},mirror);rawClient.Connect("127.0.0.1",rawHost.Port());
  bool welcome=false,ready=false;deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
  while(!ready && std::chrono::steady_clock::now()<deadline){
    rawClient.Poll();for(auto packet:rawHost.Poll()) {
      if(packet.type==Message::Hello && !welcome){rawHost.Send({Message::Welcome,0,hello});rawHost.Send({Message::Initial,0,{10}});welcome=true;}
      if(packet.type==Message::Ready){rawHost.Send({Message::Ready,0,{}});ready=true;}
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  int delivered=events;rawHost.Send({Message::Event,1,{14}});rawHost.Send({Message::Event,1,{14}});rawHost.Send({Message::Event,2,{14}});
  for(int i=0;i<100;++i){rawHost.Poll();rawClient.Poll();std::this_thread::sleep_for(std::chrono::milliseconds(1));}
  Require(ready && rawClient.State()==SessionState::Playing && events==delivered+2,"duplicate event played twice");
  rawHost.Close();rawClient.Poll();Require(rawClient.State()==SessionState::Failed,"raw host disconnect ignored");
  Transport loadingHost;loadingHost.Listen(0);NetworkSession loadingClient({"build","data"},mirror);loadingClient.Connect("127.0.0.1",loadingHost.Port());
  welcome=false;deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
  while(loadingClient.State()!=SessionState::Loading && std::chrono::steady_clock::now()<deadline){
    loadingClient.Poll();for(auto packet:loadingHost.Poll())if(packet.type==Message::Hello && !welcome){loadingHost.Send({Message::Welcome,0,hello});welcome=true;}
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  Require(loadingClient.State()==SessionState::Loading,"loading disconnect fixture never loaded");loadingHost.Close();
  Pump(loadingClient,loadingClient,[&]{return loadingClient.State()==SessionState::Failed;});
  Require(loadingClient.State()==SessionState::Failed,"disconnect during loading ignored");
  // A reader that pauses must exert backpressure, not terminate the match.
  int slowTicks=0,slowApplied=0;
  SessionCallbacks slowServer,slowMirror;
  slowServer.createInitial=[]{return Bytes{1};};
  slowServer.segment=[&]{++slowTicks;return Bytes(1024*1024,7);};
  slowMirror.applyState=[&](const Bytes& bytes,bool initial){
    Require(initial?bytes==Bytes{1}:bytes.size()==1024*1024 && bytes.front()==7 && bytes.back()==7,"slow-reader state damaged");
    if(!initial)++slowApplied;
  };
  NetworkSession slowHost({"build","data"},slowServer),slowClient({"build","data"},slowMirror);
  slowHost.Host(0);slowClient.Connect("127.0.0.1",slowHost.Port());
  Pump(slowHost,slowClient,[&]{return slowHost.State()==SessionState::Playing && slowClient.State()==SessionState::Playing;});
  auto pauseEnd=std::chrono::steady_clock::now()+std::chrono::seconds(3);
  while(std::chrono::steady_clock::now()<pauseEnd){slowHost.Poll();std::this_thread::sleep_for(std::chrono::milliseconds(1));}
  Require(slowHost.State()==SessionState::Playing && slowTicks>0 && slowTicks<55,"slow-reader backpressure failed");
  Pump(slowHost,slowClient,[&]{return slowApplied>=slowTicks;});
  Require(slowClient.State()==SessionState::Playing,"slow reader could not resume");
  // End must survive a backlog larger than one Poll's send budget.
  pauseEnd=std::chrono::steady_clock::now()+std::chrono::seconds(2);
  while(std::chrono::steady_clock::now()<pauseEnd){slowHost.Poll();std::this_thread::sleep_for(std::chrono::milliseconds(1));}
  slowHost.Finish("Backlogged match ended");
  Pump(slowHost,slowClient,[&]{
    if(slowClient.State()==SessionState::Playing)slowClient.Query({43});
    return slowClient.State()==SessionState::Finished || slowClient.State()==SessionState::Failed;
  });
  Require(slowClient.State()==SessionState::Finished && slowClient.Status()=="Backlogged match ended",
          "termination behind queued states was lost and became a receive failure");
  Pump(slowHost,slowClient,[&]{return slowHost.State()==SessionState::Finished;});
  SessionCallbacks brokenMirror;brokenMirror.applyState=[](const Bytes&,bool){throw std::runtime_error("Replica apply fixture failure");};
  NetworkSession applyHost({"build","data"},server),applyClient({"build","data"},brokenMirror);
  applyHost.Host(0);applyClient.Connect("127.0.0.1",applyHost.Port());
  Pump(applyHost,applyClient,[&]{return applyClient.State()==SessionState::Failed;});
  Require(applyClient.Status()=="Replica apply fixture failure","replica error detail lost");
  std::puts("Network sessions: readiness, compatibility, commands, previews, events, deduplication, cancellation and disconnect passed");return 0;
}catch(const std::exception& error){std::fprintf(stderr,"%s\n",error.what());return 1;}}
