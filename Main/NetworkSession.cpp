#include "NetworkSession.h"
#include "NetworkDiagnostics.h"
#include <stdexcept>
#include <utility>

namespace S2Net {
NetworkSession::NetworkSession(Compatibility identity,SessionCallbacks handlers):
  compatibility(std::move(identity)),callbacks(std::move(handlers)) {}
NetworkSession::~NetworkSession(){Finish("Player left");transport.Close();}
void NetworkSession::Log(const std::string& message) const {
  auto socket=transport.Diagnostics();
  NetworkLog(std::string(host?"host":"client")+" phase="+std::to_string(static_cast<int>(state))+" revision="+std::to_string(callbacks.revision?callbacks.revision():0)+" event="+std::to_string(host?nextEvent-1:lastEvent)+" pending="+std::to_string(pending.size())+" queued="+std::to_string(socket.queuedBytes)+" buffered="+std::to_string(socket.receiveBytes)+" operation="+socket.operation+" os="+std::to_string(socket.systemError)+" "+message);
}
Bytes NetworkSession::Hello() const {
  Bytes data;Put32(data,ProtocolVersion);PutString(data,compatibility.build);
  PutString(data,compatibility.dataFingerprint);return data;
}
void NetworkSession::Host(std::uint16_t port) {
  if(state!=SessionState::Idle) throw std::runtime_error("Session already started");
  host=true;transport.Listen(port);state=SessionState::Waiting;status="Waiting for the second player";
  Log("listen port="+std::to_string(port)+" protocol="+std::to_string(ProtocolVersion)+" build="+compatibility.build+" data="+compatibility.dataFingerprint);
  if(transport.Status()==Connection::Failed) Fail(transport.Error());
}
void NetworkSession::Connect(const std::string& ip,std::uint16_t port) {
  if(state!=SessionState::Idle) throw std::runtime_error("Session already started");
  host=false;transport.Connect(ip,port);state=SessionState::Connecting;status="Connecting";
  Log("connect ip="+ip+" port="+std::to_string(port)+" protocol="+std::to_string(ProtocolVersion)+" build="+compatibility.build+" data="+compatibility.dataFingerprint);
  deadline=std::chrono::steady_clock::now()+std::chrono::seconds(30);
  if(transport.Status()==Connection::Failed) Fail(transport.Error());
}
void NetworkSession::Fail(const std::string& error) {
  Log("FAIL "+error);
  status=error;
  auto failure=transport.Diagnostics();
  if(failure.systemError) status+=" (OS "+std::to_string(failure.systemError)+")";
  state=SessionState::Failed;pending.clear();transport.Close();
}
void NetworkSession::Finish(const std::string& reason) {
  if(state==SessionState::Finished || state==SessionState::Failed || state==SessionState::Finishing) return;
  Log("finish requested "+reason);
  status=reason;pending.clear();
  if(transport.Status()!=Connection::Connected) {state=SessionState::Finished;transport.Close();return;}
  // End follows every confirmed delta. Drain it over ordinary nonblocking
  // polls instead of closing after a single (bounded) send budget.
  try {
    Bytes data;PutString(data,reason);transport.Send({Message::End,0,data});
    state=SessionState::Finishing;
    finishDeadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    Poll();
  } catch(const std::exception& error) {
    Log(std::string("finish could not be queued: ")+error.what());
    state=SessionState::Finished;transport.Close();
  }
}
void NetworkSession::Handle(const Packet& packet) {
  if(packet.type!=Message::State && packet.type!=Message::Query && packet.type!=Message::QueryResult)
    Log("message="+std::to_string(static_cast<unsigned>(packet.type))+" id="+std::to_string(packet.sequence)+" bytes="+std::to_string(packet.data.size()));
  if(packet.type==Message::Reject || packet.type==Message::End) {
    std::size_t pos=0;auto reason=GetString(packet.data,pos);
    if(pos!=packet.data.size()) throw std::runtime_error("Invalid termination message");
    status=reason;state=packet.type==Message::Reject?SessionState::Failed:SessionState::Finished;
    transport.Close();pending.clear();return;
  }
  if(host && state==SessionState::Waiting && packet.type==Message::Hello) {
    std::size_t pos=0;auto version=Get32(packet.data,pos);
    auto build=GetString(packet.data,pos),data=GetString(packet.data,pos);
    std::string mismatch;
    if(pos!=packet.data.size() || version!=ProtocolVersion) mismatch="Incompatible network protocol";
    else if(build!=compatibility.build) mismatch="Different game builds";
    else if(data!=compatibility.dataFingerprint) mismatch="Different effective game data";
    if(!mismatch.empty()) {
      Bytes reason;PutString(reason,mismatch);transport.Send({Message::Reject,0,reason});transport.Poll();Fail(mismatch);return;
    }
    state=SessionState::Loading;status="Loading the match";
    deadline=std::chrono::steady_clock::now()+std::chrono::seconds(60);
    transport.Send({Message::Welcome,0,Hello()});
    if(!callbacks.createInitial) throw std::runtime_error("Missing host world constructor");
    transport.Send({Message::Initial,0,callbacks.createInitial()});return;
  }
  if(!host && state==SessionState::Connecting && packet.type==Message::Welcome) {
    if(packet.data!=Hello()) throw std::runtime_error("Host compatibility mismatch");
    state=SessionState::Loading;status="Receiving the match";
    deadline=std::chrono::steady_clock::now()+std::chrono::seconds(60);return;
  }
  if(!host && state==SessionState::Loading && packet.type==Message::Initial) {
    if(receivedInitial) throw std::runtime_error("Duplicate initial world");
    if(!callbacks.applyState) throw std::runtime_error("Missing client world adapter");
    callbacks.applyState(packet.data,true);
    receivedInitial=true;
    transport.Send({Message::Ready,0,{}});
    // The host acknowledges readiness before either participant may act.
    return;
  }
  if(state==SessionState::Loading && packet.type==Message::Ready && packet.data.empty()) {
    if(!host && !receivedInitial) throw std::runtime_error("Readiness before initial world");
    if(host) transport.Send({Message::Ready,0,{}});
    state=SessionState::Playing;status="Connected";
    nextTick=std::chrono::steady_clock::now()+std::chrono::milliseconds(50);return;
  }
  if(state!=SessionState::Playing) throw std::runtime_error("Unexpected message during match setup");
  if(!host && packet.type==Message::State) {
    callbacks.applyState(packet.data,false);return;
  }
  if(host && (packet.type==Message::Command || packet.type==Message::Query)) {
    if(!packet.sequence) throw std::runtime_error("Zero request ID");
    bool query=packet.type==Message::Query;
    auto& last=query?lastQuery:lastCommand;
    if(packet.sequence<=last) {
      if(!query) {
        auto result=commandResults.find(packet.sequence);
        if(result!=commandResults.end()) transport.Send({Message::CommandResult,packet.sequence,result->second});
      }
      return;
    }
    last=packet.sequence;
    auto handler=query?callbacks.query:callbacks.command;
    if(!handler) throw std::runtime_error("Missing server request handler");
    auto result=handler(1,packet.data);
    if(!query) {
      commandResults[packet.sequence]=result;
      if(commandResults.size()>256) commandResults.erase(commandResults.begin());
    }
    transport.Send({query?Message::QueryResult:Message::CommandResult,packet.sequence,result});return;
  }
  if(!host && (packet.type==Message::CommandResult || packet.type==Message::QueryResult)) {
    auto request=pending.find(packet.sequence);
    if(request==pending.end()) return;
    bool query=packet.type==Message::QueryResult;
    if(request->second!=query) throw std::runtime_error("Mismatched request result");
    pending.erase(request);
    if(callbacks.result) callbacks.result(packet.sequence,packet.data,query);return;
  }
  if(!host && packet.type==Message::Event) {
    if(packet.sequence<=lastEvent) return;
    if(packet.sequence!=lastEvent+1) throw std::runtime_error("Missing world event");
    lastEvent=packet.sequence;
    if(callbacks.event) callbacks.event(packet.data);return;
  }
  throw std::runtime_error("Unexpected network match message");
}
void NetworkSession::Poll() {
  if(state==SessionState::Idle || state==SessionState::Finished || state==SessionState::Failed) return;
  try {
    auto packets=transport.Poll();
    if(state==SessionState::Finishing) {
      if(transport.Status()==Connection::Failed ||
          std::chrono::steady_clock::now()>=finishDeadline) {
        Log("finish drained="+std::to_string(transport.Diagnostics().queuedBytes==0)+" transport="+transport.Error());
        state=SessionState::Finished;transport.Close();
      } else if(!finishWriteClosed && transport.Diagnostics().queuedBytes==0) {
        // Keep receiving in-flight previews after FIN. Closing a socket with
        // unread input can reset TCP and discard the already-sent End on Windows.
        transport.ShutdownWrite();finishWriteClosed=true;
        Log("finish write closed; waiting for peer");
      }
      return;
    }
    // TCP EOF can follow already-decoded requests and End in the same Poll.
    // Honor termination before attempting replies on the now-closed transport.
    if(transport.Status()==Connection::Failed) {
      for(const auto& packet:packets) if(packet.type==Message::End || packet.type==Message::Reject) {
        Handle(packet);return;
      }
    }
    if(!host && !helloSent && transport.Status()==Connection::Connected) {
      transport.Send({Message::Hello,0,Hello()});helloSent=true;
    }
    for(const auto& packet:packets) {
      Handle(packet);
      if(state==SessionState::Failed || state==SessionState::Finished) return;
    }
    if(transport.Status()==Connection::Failed) {Fail(transport.Error());return;}
    auto now=std::chrono::steady_clock::now();
    if(now-lastDiagnostic>std::chrono::seconds(1)){Log("status "+status);lastDiagnostic=now;}
    if(host && state==SessionState::Waiting && transport.Status()==Connection::Connected) {
      if(deadline==std::chrono::steady_clock::time_point())deadline=now+std::chrono::seconds(30);
      if(now>deadline)throw std::runtime_error("Timed out waiting for the other player");
    }
    if((state==SessionState::Connecting || state==SessionState::Loading) && now>deadline)
      throw std::runtime_error("Timed out waiting for the other player");
    if(host && state==SessionState::Playing) {
      // Bounded catch-up keeps the UI responsive. Never accelerate because
      // an individual spectator cannot see the currently executing action.
      int ticks=0;
      while(now>=nextTick && ticks++<8) {
        // Keep ordered deltas intact when the peer reads slowly. Bounded
        // backpressure pauses the common clock instead of overflowing memory.
        if(transport.Diagnostics().queuedBytes>4*1024*1024){nextTick=now+std::chrono::milliseconds(50);break;}
        nextTick+=std::chrono::milliseconds(50);
        if(callbacks.segment) transport.Send({Message::State,0,callbacks.segment()});
        if(callbacks.drainEvents) for(const auto& event:callbacks.drainEvents()) {
          transport.Send({Message::Event,nextEvent++,event});
          if(callbacks.event) callbacks.event(event);
        }
      }
    }
  }catch(const std::exception& error){Fail(error.what());}
}
std::uint64_t NetworkSession::Request(const Bytes& data,bool query) {
  if(state!=SessionState::Playing) return 0;
  if(pending.size()>=1024) return 0;
  auto id=nextRequest++;
  if(host) {
    try {
      auto handler=query?callbacks.query:callbacks.command;
      if(!handler) return 0;
      auto result=handler(0,data);
      if(callbacks.result) callbacks.result(id,result,query);
    }catch(const std::exception& error){Fail(error.what());return 0;}
    }else {
      try {
        pending[id]=query;transport.Send({query?Message::Query:Message::Command,id,data});
      }catch(const std::exception& error){Fail(error.what());return 0;}
  }
  return id;
}
}
