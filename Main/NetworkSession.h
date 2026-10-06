#pragma once
#include "NetworkTransport.h"
#include <chrono>
#include <functional>
#include <map>

namespace S2Net {
enum class SessionState { Idle, Waiting, Connecting, Loading, Playing, Finishing, Finished, Failed };
struct Compatibility {
  std::string build;
  std::string dataFingerprint;
};
struct SessionCallbacks {
  std::function<Bytes()> createInitial;
  std::function<void(const Bytes&,bool)> applyState;
  std::function<Bytes()> segment;
  std::function<Bytes(unsigned,const Bytes&)> command;
  std::function<Bytes(unsigned,const Bytes&)> query;
  std::function<std::vector<Bytes>()> drainEvents;
  std::function<void(const Bytes&)> event;
  std::function<void(std::uint64_t,const Bytes&,bool)> result;
  std::function<std::uint64_t()> revision;
};
// Owned by the connection screen, then by the mission. Poll never waits on I/O.
class NetworkSession {
  Transport transport;
  Compatibility compatibility;
  SessionCallbacks callbacks;
  SessionState state=SessionState::Idle;
  bool host=false, helloSent=false, receivedInitial=false, finishWriteClosed=false;
  std::string status;
  std::uint64_t nextRequest=1, nextEvent=1, lastEvent=0, lastCommand=0, lastQuery=0;
  std::map<std::uint64_t,Bytes> commandResults;
  std::map<std::uint64_t,bool> pending;
  std::chrono::steady_clock::time_point deadline,nextTick,finishDeadline;
  std::chrono::steady_clock::time_point lastDiagnostic;
  void Log(const std::string&) const;
  void Handle(const Packet&);
  void Fail(const std::string&);
  Bytes Hello() const;
  std::uint64_t Request(const Bytes&,bool);
 public:
  NetworkSession(Compatibility,SessionCallbacks);
  ~NetworkSession();
  void Host(std::uint16_t port=DefaultPort);
  void Connect(const std::string& ip,std::uint16_t port=DefaultPort);
  void Poll();
  void Finish(const std::string& reason);
  std::uint64_t Submit(const Bytes& action) { return Request(action,false); }
  std::uint64_t Query(const Bytes& preview) { return Request(preview,true); }
  unsigned LocalSlot() const { return host?0:1; }
  bool IsHost() const { return host; }
  SessionState State() const { return state; }
  const std::string& Status() const { return status; }
  std::uint16_t Port() const { return transport.Port(); }
};
}
