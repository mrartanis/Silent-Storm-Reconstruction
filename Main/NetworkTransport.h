#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace S2Net {
using Bytes = std::vector<std::uint8_t>;
constexpr std::uint32_t ProtocolVersion = 2;
constexpr std::uint16_t DefaultPort = 7780;
enum class Message : std::uint32_t {
  Hello = 1, Welcome, Initial, Ready, State, Command, CommandResult,
  Query, QueryResult, Event, End, Reject, Ping, Pong
};
struct Packet { Message type; std::uint64_t sequence = 0; Bytes data; };
void Put32(Bytes&, std::uint32_t);
void Put64(Bytes&, std::uint64_t);
std::uint32_t Get32(const Bytes&, std::size_t&);
std::uint64_t Get64(const Bytes&, std::size_t&);
void PutString(Bytes&, const std::string&);
std::string GetString(const Bytes&, std::size_t&);
Bytes Encode(const Packet&);
// TCP boundaries have no relation to packet boundaries.
class Decoder {
  Bytes pending;
 public:
  std::vector<Packet> Feed(const void*, std::size_t);
  std::size_t BufferedBytes() const {return pending.size();}
};
enum class Connection { Closed, Listening, Connecting, Connected, Failed };
struct TransportDiagnostics {int systemError=0;std::string operation;std::size_t queuedBytes=0,receiveBytes=0;};
class Transport {
  struct Impl;
  std::unique_ptr<Impl> impl;
 public:
  Transport();
  ~Transport();
  Transport(const Transport&) = delete;
  Transport& operator=(const Transport&) = delete;
  void Listen(std::uint16_t port);
  void Connect(const std::string& ip, std::uint16_t port);
  void Send(const Packet&);
  std::vector<Packet> Poll();
  void Close();
  void ShutdownWrite(); // Half-close only after every queued byte was sent.
  Connection Status() const;
  const std::string& Error() const;
  std::uint16_t Port() const;
  TransportDiagnostics Diagnostics() const;
};
}
