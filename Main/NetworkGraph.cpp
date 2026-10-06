#include "NetworkGraph.h"
#include "../FileIO/PortableStructureChunks.h"
#include <limits>
#include <stdexcept>
#include <set>

namespace S2Net {
namespace {
constexpr std::size_t MaxObjects = 1000000;
void Blob(Bytes& output,const Bytes& input) {
  Put32(output,static_cast<std::uint32_t>(input.size()));
  output.insert(output.end(),input.begin(),input.end());
}
Bytes Blob(const Bytes& input,std::size_t& pos) {
  auto length = Get32(input,pos);
  if(length > input.size()-pos) throw std::runtime_error("Truncated object record");
  Bytes result(input.begin()+pos,input.begin()+pos+length); pos += length; return result;
}
void Chunk(Bytes& output,std::uint8_t id,const Bytes& payload) {
  if(payload.size()>std::numeric_limits<std::uint32_t>::max()/2)
    throw std::runtime_error("World record too large");
  output.push_back(id);
  auto size=static_cast<std::uint32_t>(payload.size());
  if(size < 128) output.push_back(static_cast<std::uint8_t>(size<<1));
  else Put32(output,(size<<1)|1);
  output.insert(output.end(),payload.begin(),payload.end());
}
std::map<unsigned,Bytes> Chunks(const Bytes& stream) {
  std::map<unsigned,Bytes> result;
  std::size_t offset=0;
  while(offset<stream.size()) {
    S2FileIO::StructureChunk chunk{};
    if(!S2FileIO::DecodeStructureChunkAt(stream.data(),stream.size(),offset,&chunk))
      throw std::runtime_error("Malformed world structure");
    if(result.count(chunk.id)) throw std::runtime_error("Duplicate structure chunk");
    result[chunk.id]=Bytes(stream.begin()+chunk.payloadOffset,stream.begin()+chunk.payloadOffset+chunk.length);
    offset=static_cast<std::size_t>(chunk.payloadOffset)+chunk.length;
  }
  return result;
}
}
Bytes NetworkGraph::Capture(const Bytes& structure,bool initial) {
  auto chunks=Chunks(structure);
  if(chunks.count(3) || !chunks.count(0) || !chunks.count(1) || !chunks.count(2))
    throw std::runtime_error("Unsupported network world structure");
  std::vector<S2FileIO::StructureObjectRecord> table;
  std::vector<S2FileIO::StructureObjectBody> bodies;
  const auto& tableBytes=chunks.at(0); const auto& data=chunks.at(2);
  if(!S2FileIO::DecodeStructureObjectTable(tableBytes.data(),tableBytes.size(),&table) ||
     !S2FileIO::IndexStructureObjectBodies(data.data(),data.size(),&bodies) ||
     table.size()!=bodies.size() || table.size()>MaxObjects)
    throw std::runtime_error("Invalid network object table");
  std::map<NetObjectId,ObjectRecord> current;
  for(const auto& entry: table) {
    if(!entry.wireId || current.count(entry.wireId)) throw std::runtime_error("Duplicate network object ID");
    current[entry.wireId]={entry.typeId,entry.valid,{}};
  }
  std::set<NetObjectId> seen;
  for(const auto& body: bodies) {
    if(!current.count(body.wireId) || !seen.insert(body.wireId).second)
      throw std::runtime_error("Invalid network object body");
    current.at(body.wireId).body=Bytes(data.begin()+body.bodyOffset,data.begin()+body.bodyOffset+body.bodyLength);
  }
  if(initial) { records.clear(); revision=0; }
  Bytes delta; Put64(delta,revision); Put64(delta,revision+1); Blob(delta,chunks.at(1));
  std::vector<NetObjectId> updates,deleted;
  for(const auto& entry:current) {
    auto prior=records.find(entry.first);
    if(prior==records.end() || !(prior->second==entry.second)) updates.push_back(entry.first);
    if(prior!=records.end() && prior->second.type!=entry.second.type)
      throw std::runtime_error("Network object type changed");
  }
  for(const auto& entry:records) if(!current.count(entry.first)) deleted.push_back(entry.first);
  Put32(delta,static_cast<std::uint32_t>(updates.size()));
  for(auto id:updates) {
    const auto& entry=current.at(id);
    Put32(delta,id); Put32(delta,entry.type); Put32(delta,entry.valid ? 1 : 0); Blob(delta,entry.body);
  }
  Put32(delta,static_cast<std::uint32_t>(deleted.size()));
  for(auto id:deleted) Put32(delta,id);
  records.swap(current); root=chunks.at(1); ++revision;
  return delta;
}
void NetworkGraph::Apply(const Bytes& delta,bool initial) {
  std::size_t pos=0;
  auto base=Get64(delta,pos), next=Get64(delta,pos);
  if(base!=(initial?0:revision) || next!=base+1)
    throw std::runtime_error("Out of order world revision");
  auto nextRoot=Blob(delta,pos);
  auto current=initial ? std::map<NetObjectId,ObjectRecord>() : records;
  std::set<NetObjectId> touched;
  auto count=Get32(delta,pos);
  if(count>MaxObjects) throw std::runtime_error("Too many network objects");
  for(std::uint32_t i=0;i<count;++i) {
    auto id=Get32(delta,pos),type=Get32(delta,pos),valid=Get32(delta,pos);
    if(!id || valid>1 || !touched.insert(id).second)
      throw std::runtime_error("Duplicate or invalid network record");
    auto prior=current.find(id);
    if(prior!=current.end() && prior->second.type!=type)
      throw std::runtime_error("Network object type changed");
    current[id]={type,valid==1,Blob(delta,pos)};
  }
  count=Get32(delta,pos);
  if(count>MaxObjects) throw std::runtime_error("Too many network deletions");
  for(std::uint32_t i=0;i<count;++i) {
    auto id=Get32(delta,pos);
    if(!id || !touched.insert(id).second || !current.erase(id))
      throw std::runtime_error("Invalid network deletion");
  }
  if(pos!=delta.size() || current.size()>MaxObjects) throw std::runtime_error("Invalid world packet size");
  records.swap(current); root.swap(nextRoot); revision=next;
}
Bytes NetworkGraph::Structure() const {
  Bytes output,version,table,data;
  Put32(version,1); Chunk(output,4,version); Chunk(output,1,root);
  for(const auto& item:records) {
    Put32(table,item.second.type); Put32(table,item.first); table.push_back(item.second.valid?1:0);
    Bytes body,id; Put32(id,item.first); Chunk(body,0,id); Chunk(body,1,item.second.body); Chunk(data,1,body);
  }
  Chunk(output,0,table); Chunk(output,2,data); return output;
}
}
