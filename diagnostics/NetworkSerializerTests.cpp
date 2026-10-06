#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Main/NetworkGraph.h"
#include <cstdio>
#include <algorithm>
#include <stdexcept>
class NetworkBareBase: public CObjectBase {
  OBJECT_BASIC_METHODS(NetworkBareBase);
};
class NetworkTestNode: public NetworkBareBase {
  OBJECT_BASIC_METHODS(NetworkTestNode);
 public:
  int value=0;
  CObj<NetworkTestNode> child;
  CPtr<NetworkTestNode> parent;
  std::vector<CMObj<NetworkTestNode>> squad;
  int reads=0;
  int operator&(CStructureSaver& f) {
    if(f.IsReading()) ++reads;
    f.Add(1,&value); f.Add(2,&child); f.Add(3,&parent); f.Add(4,&squad);
    f.Add(5,static_cast<NetworkBareBase*>(this));return 0;
  }
};
REGISTER_SAVELOAD_CLASS(0x7efd0001,NetworkTestNode)
static void Require(bool condition,const char* msg) { if(!condition) throw std::runtime_error(msg); }
int main() {
  try {
    CStructureNetworkContext host,client;
    S2Net::NetworkGraph output,input;
    CObj<NetworkTestNode> source=new NetworkTestNode, replica;
    source->value=5; source->child=new NetworkTestNode; source->child->value=5;
    source->child->parent=source;
    source->squad.push_back(new NetworkTestNode);
    auto sync=[&](bool initial) {
      CMemoryStream bytes;
      {CStructureSaver save(bytes,CStructureSaver::WRITE,&host); save.Add(1,&source);}
      S2Net::Bytes wire(bytes.GetBuffer(),bytes.GetBuffer()+bytes.GetSize());
      S2Net::Bytes nativeVtable(reinterpret_cast<const unsigned char*>(source.GetPtr()),
                              reinterpret_cast<const unsigned char*>(source.GetPtr())+sizeof(void*));
      Require(std::search(wire.begin(),wire.end(),nativeVtable.begin(),nativeVtable.end())==wire.end(),
              "empty polymorphic base leaked a native vtable into the protocol");
      input.Apply(output.Capture(wire,initial),initial);
      auto assembled=input.Structure(); CMemoryStream received;
      received.Write(assembled.data(),static_cast<int>(assembled.size())); received.Seek(0);
      {CStructureSaver load(received,CStructureSaver::READ,&client); load.Add(1,&replica);}
    };
    sync(true);
    auto* root=replica.GetPtr(); auto* child=replica->child.GetPtr();
    Require(root!=source.GetPtr() && child!=source->child.GetPtr(),"independent graph instances");
    Require(replica->child->parent.GetPtr()==root,"cyclic reference reconstruction");
    Require(host.ids.at(source.GetPtr())!=host.ids.at(source->child.GetPtr()),"equal values require distinct IDs");
    int reads=root->reads;
    sync(false); Require(root->reads==reads,"unchanged record reapplied");
    Require(client.changed.empty(),"unchanged graph emitted presentation changes");
    source->child->value=17; sync(false);
    Require(replica.GetPtr()==root && replica->child.GetPtr()==child,"replica identities changed");
    Require(child->value==17 && child->parent.GetPtr()==root,"in-place update broken");
    Require(std::find(client.changed.begin(),client.changed.end(),child)!=client.changed.end(),"in-place change did not notify presentation");
    auto* member=replica->squad.front().GetPtr();
    source->squad.push_back(new NetworkTestNode); sync(false);
    Require(IsValid(member) && replica->squad.front().GetPtr()==member,
            "rebuilding a mission-owned list invalidated an unchanged member");
    auto retiredID=host.ids.at(source->child.GetPtr());
    CPtr<NetworkTestNode> selected=child;
    CObj<NetworkTestNode> presentationOwner=child;
    source->child=0; sync(false);
    Require(!replica->child && !client.refs.count(retiredID),"deleted object retained in replica");
    Require(IsValid(presentationOwner) && presentationOwner->value==17,
            "replica deletion invalidated an object still owned by presentation");
    presentationOwner=0;
    Require(!IsValid(selected),"deleted object outlived its final presentation owner");
    source->child=new NetworkTestNode; sync(false);
    Require(host.ids.at(source->child.GetPtr())>retiredID,"object ID reused");
    Require(replica.GetPtr()==root,"selected root identity lost");
    CObj<NetworkTestNode> tombstoneOwner=replica->child.GetPtr();
    source->child->Invalidate();sync(false);
    Require(!IsValid(tombstoneOwner) && !IsValid(replica->child),
            "server tombstone did not invalidate existing presentation references");
    source->child=new NetworkTestNode;sync(false);
    // Ordinary save writers remain independent of the session registry.
    CMemoryStream file;
    {CStructureSaver save(file,CStructureSaver::WRITE);save.Add(1,&source);}
    file.Seek(0); CObj<NetworkTestNode> restored;
    {CStructureSaver load(file,CStructureSaver::READ);load.Add(1,&restored);}
    Require(restored && restored->value==source->value && restored->child,"ordinary save regression");
    source->child->Invalidate();
    CStructureNetworkContext transientWriter,transientReader;
    transientWriter.externalReferences=transientReader.externalReferences=true;
    transientWriter.nextID=0x80000000u;
    CMemoryStream malformed;
    {CStructureSaver save(malformed,CStructureSaver::WRITE,&transientWriter);save.Add(1,&source);}
    malformed.Seek(0);bool rejected=false;
    try {CStructureSaver load(malformed,CStructureSaver::READ,&transientReader);load.Add(1,&restored);}
    catch(const SFileIOError&) {rejected=true;}
    Require(rejected,"invalid transient object accepted as a live request");
    std::puts("Network serializer: stable IDs, cycles, in-place changes, unchanged records, deletions, ordinary saves passed");
    return 0;
  }catch(const SFileIOError& e){std::fprintf(stderr,"%s\n",e.szError.c_str());}
   catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());}
  return 1;
}
