#include "NetworkGraph.h"
#include "../FileIO/PortableStructureChunks.h"
#include <limits>
#include <stdexcept>
#include <set>
#include <unordered_map>
#include <cstring>

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
struct ByteView { const std::uint8_t* data; std::size_t size; };
std::map<unsigned,ByteView> Chunks(const std::uint8_t* stream,std::size_t size) {
  std::map<unsigned,ByteView> result;
  std::size_t offset=0;
  while(offset<size) {
    S2FileIO::StructureChunk chunk{};
    if(!S2FileIO::DecodeStructureChunkAt(stream,size,offset,&chunk))
      throw std::runtime_error("Malformed world structure");
    if(result.count(chunk.id)) throw std::runtime_error("Duplicate structure chunk");
    result[chunk.id]={stream+chunk.payloadOffset,chunk.length};
    offset=static_cast<std::size_t>(chunk.payloadOffset)+chunk.length;
  }
  return result;
}
// Validation and allocations happen before committing. Moving existing bodies
// and transferring new map nodes do not allocate or copy unchanged records.
void Commit(std::map<NetObjectId,ObjectRecord>& records,std::map<NetObjectId,ObjectRecord>& updates,
            const std::vector<NetObjectId>& deleted,bool initial) {
  if(initial)records.clear();
  for(auto& entry:updates) {
    auto existing=records.find(entry.first);
    if(existing!=records.end())existing->second=std::move(entry.second);
  }
  records.merge(updates);
  for(auto id:deleted)records.erase(id);
}
}

Bytes NetworkGraph::Capture(const Bytes& structure,bool initial) {
  return Capture(structure.data(),structure.size(),initial);
}
Bytes NetworkGraph::Capture(const std::uint8_t* structure,std::size_t size,bool initial) {
  auto chunks=Chunks(structure,size);
  if(chunks.count(3) || !chunks.count(0) || !chunks.count(1) || !chunks.count(2))
    throw std::runtime_error("Unsupported network world structure");
  std::vector<S2FileIO::StructureObjectRecord> table;
  std::vector<S2FileIO::StructureObjectBody> bodies;
  const auto& tableBytes=chunks.at(0);const auto& data=chunks.at(2);
  if(!S2FileIO::DecodeStructureObjectTable(tableBytes.data,tableBytes.size,&table) ||
     !S2FileIO::IndexStructureObjectBodies(data.data,data.size,&bodies) ||
     table.size()!=bodies.size() || table.size()>MaxObjects)
    throw std::runtime_error("Invalid network object table");
  std::unordered_map<NetObjectId,const S2FileIO::StructureObjectRecord*> current;
  current.reserve(table.size());
  for(const auto& entry:table) {
    if(!entry.wireId || !current.emplace(entry.wireId,&entry).second)
      throw std::runtime_error("Duplicate network object ID");
  }
  std::set<NetObjectId> seen,nextUpdated;
  std::map<NetObjectId,ObjectRecord> updates;
  for(const auto& body:bodies) {
    auto found=current.find(body.wireId);
    if(found==current.end() || !seen.insert(body.wireId).second)
      throw std::runtime_error("Invalid network object body");
    const auto& entry=*found->second;
    auto prior=records.find(body.wireId);
    if(!initial && prior!=records.end() && prior->second.type!=entry.typeId)
      throw std::runtime_error("Network object type changed");
    const auto* bytes=data.data+body.bodyOffset;
    if(initial || prior==records.end() || prior->second.valid!=entry.valid ||
       prior->second.body.size()!=body.bodyLength ||
       (body.bodyLength && std::memcmp(prior->second.body.data(),bytes,body.bodyLength)!=0)) {
      updates[body.wireId]={entry.typeId,entry.valid,Bytes(bytes,bytes+body.bodyLength)};
      nextUpdated.insert(body.wireId);
    }
  }
  std::vector<NetObjectId> deleted;
  if(!initial)for(const auto& entry:records)if(!current.count(entry.first))deleted.push_back(entry.first);
  const auto& rootView=chunks.at(1);Bytes nextRoot(rootView.data,rootView.data+rootView.size);
  const auto base=initial?0:revision;
  Bytes delta;Put64(delta,base);Put64(delta,base+1);Blob(delta,nextRoot);
  Put32(delta,static_cast<std::uint32_t>(updates.size()));
  for(const auto& entry:updates) {
    Put32(delta,entry.first);Put32(delta,entry.second.type);Put32(delta,entry.second.valid?1:0);Blob(delta,entry.second.body);
  }
  Put32(delta,static_cast<std::uint32_t>(deleted.size()));
  for(auto id:deleted)Put32(delta,id);
  Commit(records,updates,deleted,initial);root.swap(nextRoot);updated.swap(nextUpdated);revision=base+1;
  return delta;
}

void NetworkGraph::Apply(const Bytes& delta,bool initial) {
  std::size_t pos=0;
  auto base=Get64(delta,pos), next=Get64(delta,pos);
  if(base!=(initial?0:revision) || next!=base+1)
    throw std::runtime_error("Out of order world revision");
  auto nextRoot=Blob(delta,pos);
  std::map<NetObjectId,ObjectRecord> updates;
  std::set<NetObjectId> touched,nextUpdated;
  std::size_t additions=0;
  auto count=Get32(delta,pos);
  if(count>MaxObjects)throw std::runtime_error("Too many network objects");
  for(std::uint32_t i=0;i<count;++i) {
    auto id=Get32(delta,pos),type=Get32(delta,pos),valid=Get32(delta,pos);
    if(!id || valid>1 || !touched.insert(id).second)
      throw std::runtime_error("Duplicate or invalid network record");
    auto prior=records.find(id);
    if(!initial && prior!=records.end() && prior->second.type!=type)
      throw std::runtime_error("Network object type changed");
    if(initial || prior==records.end())++additions;
    updates[id]={type,valid==1,Blob(delta,pos)};nextUpdated.insert(id);
  }
  count=Get32(delta,pos);
  if(count>MaxObjects)throw std::runtime_error("Too many network deletions");
  std::vector<NetObjectId> deleted;deleted.reserve(count);
  for(std::uint32_t i=0;i<count;++i) {
    auto id=Get32(delta,pos);
    if(!id || !touched.insert(id).second || initial || !records.count(id))
      throw std::runtime_error("Invalid network deletion");
    deleted.push_back(id);
  }
  if(pos!=delta.size() || (initial?0:records.size())+additions-deleted.size()>MaxObjects)
    throw std::runtime_error("Invalid world packet size");
  Commit(records,updates,deleted,initial);root.swap(nextRoot);updated.swap(nextUpdated);revision=next;
}

Bytes NetworkGraph::Structure(bool changedOnly) const {
  Bytes output,version,table,data;
  Put32(version,1); Chunk(output,4,version); Chunk(output,1,root);
  for(const auto& item:records) {
    Put32(table,item.second.type); Put32(table,item.first); table.push_back(item.second.valid?1:0);
    Bytes body,id; Put32(id,item.first); Chunk(body,0,id);
    if(!changedOnly || updated.count(item.first))Chunk(body,1,item.second.body);
    Chunk(data,1,body);
  }
  Chunk(output,0,table); Chunk(output,2,data); return output;
}
}
