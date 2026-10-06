#include "NetworkTransport.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <deque>
#include <stdexcept>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <cerrno>
#endif

namespace S2Net {
namespace {
constexpr std::uint32_t Magic = 0x534e3253;
constexpr std::size_t MaxPacket = 64 * 1024 * 1024;
#ifdef _WIN32
using Socket = SOCKET;
constexpr Socket Invalid = INVALID_SOCKET;
int LastError() { return WSAGetLastError(); }
bool Pending(int e) { return e == WSAEWOULDBLOCK || e == WSAEINPROGRESS || e == WSAEINTR; }
void CloseSocket(Socket s) { if (s != Invalid) closesocket(s); }
void Nonblock(Socket s) { u_long yes = 1; if (ioctlsocket(s, FIONBIO, &yes)) throw std::runtime_error("Cannot make socket nonblocking"); }
struct Startup { Startup() { WSADATA d; if (WSAStartup(MAKEWORD(2,2), &d)) throw std::runtime_error("Winsock initialization failed"); } ~Startup() { WSACleanup(); } };
#else
using Socket = int;
constexpr Socket Invalid = -1;
int LastError() { return errno; }
bool Pending(int e) { return e == EWOULDBLOCK || e == EAGAIN || e == EINPROGRESS || e == EINTR; }
void CloseSocket(Socket s) { if (s != Invalid) close(s); }
void Nonblock(Socket s) { if (fcntl(s, F_SETFL, fcntl(s, F_GETFL) | O_NONBLOCK) < 0) throw std::runtime_error("Cannot make socket nonblocking"); }
#endif
}
void Put32(Bytes& b, std::uint32_t v) { for (unsigned i=0;i<4;++i) b.push_back(static_cast<std::uint8_t>(v >> (i*8))); }
void Put64(Bytes& b, std::uint64_t v) { Put32(b, static_cast<std::uint32_t>(v)); Put32(b, static_cast<std::uint32_t>(v >> 32)); }
std::uint32_t Get32(const Bytes& b, std::size_t& p) {
  if (p > b.size() || b.size()-p < 4) throw std::runtime_error("Truncated network field");
  std::uint32_t v=0; for (unsigned i=0;i<4;++i) v |= std::uint32_t(b[p++]) << (i*8); return v;
}
std::uint64_t Get64(const Bytes& b, std::size_t& p) { auto lo=Get32(b,p); return lo | (std::uint64_t(Get32(b,p)) << 32); }
void PutString(Bytes& b, const std::string& s) { Put32(b, static_cast<std::uint32_t>(s.size())); b.insert(b.end(),s.begin(),s.end()); }
std::string GetString(const Bytes& b, std::size_t& p) { auto n=Get32(b,p); if(n > b.size()-p) throw std::runtime_error("Truncated network string"); std::string s(b.begin()+p,b.begin()+p+n); p+=n; return s; }
Bytes Encode(const Packet& p) {
  if (p.data.size()>MaxPacket) throw std::runtime_error("Network packet too large");
  Bytes b; Put32(b,Magic); Put32(b,ProtocolVersion); Put32(b,static_cast<std::uint32_t>(p.type)); Put32(b,static_cast<std::uint32_t>(p.data.size())); Put64(b,p.sequence); b.insert(b.end(),p.data.begin(),p.data.end()); return b;
}
std::vector<Packet> Decoder::Feed(const void* raw, std::size_t count) {
  if (count) { const auto* p=static_cast<const std::uint8_t*>(raw); pending.insert(pending.end(),p,p+count); }
  std::vector<Packet> out; std::size_t consumed=0;
  while (pending.size()-consumed>=24) {
    std::size_t p=consumed;
    if(Get32(pending,p)!=Magic || Get32(pending,p)!=ProtocolVersion) throw std::runtime_error("Incompatible network protocol");
    auto t=Get32(pending,p), n=Get32(pending,p); auto seq=Get64(pending,p);
    if(n>MaxPacket || t<1 || t>static_cast<std::uint32_t>(Message::Pong)) throw std::runtime_error("Invalid network packet");
    if(pending.size()-p<n) break;
    out.push_back({static_cast<Message>(t),seq,Bytes(pending.begin()+p,pending.begin()+p+n)}); consumed=p+n;
  }
  pending.erase(pending.begin(),pending.begin()+consumed);
  if(pending.size()>MaxPacket+24) throw std::runtime_error("Network receive buffer overflow");
  return out;
}
struct Transport::Impl {
#ifdef _WIN32
  Startup startup;
#endif
  Socket listener=Invalid, peer=Invalid;
  Connection status=Connection::Closed;
  std::string error;
  TransportDiagnostics diagnostic;
  std::uint16_t port=0;
  Decoder decoder;
  std::deque<Bytes> writes;
  std::size_t offset=0, queued=0;
  std::chrono::steady_clock::time_point connecting;
  void Fail(const std::string& s) {error=s;diagnostic.queuedBytes=queued;diagnostic.receiveBytes=decoder.BufferedBytes();CloseSocket(peer);peer=Invalid;CloseSocket(listener);listener=Invalid;writes.clear();queued=offset=0;status=Connection::Failed;}
};
Transport::Transport():impl(new Impl) {}
Transport::~Transport() { Close(); }
void Transport::Close() { CloseSocket(impl->peer); CloseSocket(impl->listener); impl->peer=impl->listener=Invalid; impl->writes.clear(); impl->offset=impl->queued=0; impl->decoder=Decoder(); impl->status=Connection::Closed; }
void Transport::ShutdownWrite() {
  if(impl->status!=Connection::Connected || impl->queued) throw std::runtime_error("Cannot finish an undrained connection");
  impl->diagnostic.operation="shutdown-send";
#ifdef _WIN32
  int result=shutdown(impl->peer,SD_SEND);
#else
  int result=shutdown(impl->peer,SHUT_WR);
#endif
  if(result){impl->diagnostic.systemError=LastError();throw std::runtime_error("Connection shutdown failed");}
}
void Transport::Listen(std::uint16_t port) {
  Close(); impl->error.clear();
  impl->diagnostic={};impl->diagnostic.operation="listen";
  try {
    impl->listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP); if(impl->listener==Invalid) throw std::runtime_error("Cannot create listening socket");
    int reuse=1;
#ifdef _WIN32
    if(setsockopt(impl->listener,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,reinterpret_cast<const char*>(&reuse),sizeof(reuse)))
#else
    // WSL1 maps SO_REUSEADDR to Winsock's permissive sharing semantics:
    // it would allow a second host to steal an already listening port.
    // Native Linux keeps SO_REUSEADDR for normal TIME_WAIT restarts.
    utsname platform{};
    if(uname(&platform)==0 && (std::strstr(platform.release,"Microsoft") || std::strstr(platform.release,"microsoft")))reuse=0;
    if(setsockopt(impl->listener,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse)))
#endif
      throw std::runtime_error("Cannot configure listening socket");
    Nonblock(impl->listener); sockaddr_in a{}; a.sin_family=AF_INET; a.sin_port=htons(port); a.sin_addr.s_addr=htonl(INADDR_ANY);
    if(bind(impl->listener,reinterpret_cast<sockaddr*>(&a),sizeof(a)) || listen(impl->listener,4)){impl->diagnostic.systemError=LastError();throw std::runtime_error("Cannot listen: port is unavailable");}
#ifdef _WIN32
    int size=sizeof(a);
#else
    socklen_t size=sizeof(a);
#endif
    getsockname(impl->listener,reinterpret_cast<sockaddr*>(&a),&size); impl->port=ntohs(a.sin_port); impl->status=Connection::Listening;
  } catch(const std::exception& e) { impl->Fail(e.what()); }
}
void Transport::Connect(const std::string& ip, std::uint16_t port) {
  Close(); impl->error.clear();
  impl->diagnostic={};impl->diagnostic.operation="connect";
  try {
    sockaddr_in a{}; a.sin_family=AF_INET; a.sin_port=htons(port);
    if(!port || inet_pton(AF_INET,ip.c_str(),&a.sin_addr)!=1) throw std::runtime_error("Enter a numeric IPv4 address and port 1..65535");
    impl->peer=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP); if(impl->peer==Invalid) throw std::runtime_error("Cannot create connection socket"); Nonblock(impl->peer);
    int r=connect(impl->peer,reinterpret_cast<sockaddr*>(&a),sizeof(a));
    if(r){int error=LastError();if(!Pending(error)){impl->diagnostic.systemError=error;throw std::runtime_error("Connection refused");}}
    impl->status=r ? Connection::Connecting : Connection::Connected; impl->port=port; impl->connecting=std::chrono::steady_clock::now();
  } catch(const std::exception& e) { impl->Fail(e.what()); }
}
void Transport::Send(const Packet& p) {
  if(impl->status!=Connection::Connected) throw std::runtime_error("Not connected");
  auto bytes=Encode(p); if(impl->queued+bytes.size()>MaxPacket*2) throw std::runtime_error("Network send queue overflow"); impl->queued+=bytes.size(); impl->writes.push_back(std::move(bytes));
}
std::vector<Packet> Transport::Poll() {
  std::vector<Packet> out;
  try {
    if(impl->listener!=Invalid) {
      auto s=accept(impl->listener,nullptr,nullptr);
      if(s!=Invalid) {
        if(impl->peer!=Invalid) {
          Nonblock(s);Bytes reason;PutString(reason,"Match already has two players");auto rejection=Encode({Message::Reject,0,reason});
#ifdef _WIN32
          send(s,reinterpret_cast<const char*>(rejection.data()),static_cast<int>(rejection.size()),0);
#else
          send(s,reinterpret_cast<const char*>(rejection.data()),static_cast<int>(rejection.size()),MSG_NOSIGNAL);
#endif
          CloseSocket(s);
        }
        else { Nonblock(s); impl->peer=s; impl->status=Connection::Connected; }
      } else if(!Pending(LastError())) throw std::runtime_error("Accept failed");
    }
    if(impl->status==Connection::Connecting) {
      fd_set w,e; FD_ZERO(&w); FD_ZERO(&e); FD_SET(impl->peer,&w); FD_SET(impl->peer,&e); timeval zero{};
      if(select(static_cast<int>(impl->peer)+1,nullptr,&w,&e,&zero)>0) {
        int error=0;
#ifdef _WIN32
        int n=sizeof(error);
#else
        socklen_t n=sizeof(error);
#endif
        getsockopt(impl->peer,SOL_SOCKET,SO_ERROR,reinterpret_cast<char*>(&error),&n);
        if(error || FD_ISSET(impl->peer,&e)){impl->diagnostic.systemError=error;throw std::runtime_error("Connection refused");} impl->status=Connection::Connected;
      } else if(std::chrono::steady_clock::now()-impl->connecting>std::chrono::seconds(10)) throw std::runtime_error("Connection timed out");
    }
    if(impl->status!=Connection::Connected) return out;
    // Limit work per frame, including fast local connections.
    for(int i=0;i<16 && !impl->writes.empty();++i) {
      const auto& b=impl->writes.front();
#ifdef _WIN32
      int flags=0;
#else
      int flags=MSG_NOSIGNAL;
#endif
      int n=send(impl->peer,reinterpret_cast<const char*>(b.data()+impl->offset),static_cast<int>(std::min<std::size_t>(b.size()-impl->offset,65536)),flags);
      if(n<0){int error=LastError();if(Pending(error))break;impl->diagnostic.systemError=error;impl->diagnostic.operation="send";throw std::runtime_error("Send failed");}
      if(!n) throw std::runtime_error("Connection closed");
      impl->offset+=n; impl->queued-=n;
      if(impl->offset==b.size()) { impl->writes.pop_front(); impl->offset=0; }
    }
    for(int i=0;i<16;++i) {
      char b[65536]; int n=recv(impl->peer,b,sizeof(b),0);
      if(n<0){int error=LastError();if(Pending(error))break;impl->diagnostic.systemError=error;impl->diagnostic.operation="recv";throw std::runtime_error("Receive failed");}
      if(!n){impl->diagnostic.operation="recv-eof";throw std::runtime_error("Other player disconnected");}
      impl->diagnostic.operation="decode";
      auto packets=impl->decoder.Feed(b,n); for(auto& p:packets) out.push_back(std::move(p));
    }
  } catch(const std::exception& e) { impl->Fail(e.what()); }
  return out;
}
Connection Transport::Status() const { return impl->status; }
const std::string& Transport::Error() const { return impl->error; }
std::uint16_t Transport::Port() const { return impl->port; }
TransportDiagnostics Transport::Diagnostics() const {auto result=impl->diagnostic;if(impl->status!=Connection::Failed){result.queuedBytes=impl->queued;result.receiveBytes=impl->decoder.BufferedBytes();}return result;}
}
