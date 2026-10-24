#include "../Main/NetworkIdentity.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

int main(int argc,char** argv) {
  try {
    if(argc!=3)throw std::runtime_error("Expected build ID and scratch directory");
    const std::string build=argv[1],data="silent-storm-1.2";
    const auto root=std::filesystem::path(argv[2]);std::filesystem::create_directories(root);
    std::atomic<bool> cancelled=false;
    auto write=[&](std::string text){std::ofstream(root/"network-version.txt")<<text;};
    auto identify=[&]{return S2Net::IdentifyGameData({"resources-that-do-not-exist"},{(root/"game.db").string()},cancelled);};
    auto rejected=[&]{try{identify();}catch(const std::exception&){return true;}return false;};
    std::filesystem::remove(root/"network-version.txt");
    if(!rejected())throw std::runtime_error("Missing manifest accepted");
    write("S2-NETWORK-1\n"+build+"\n"+data+"\n");
    if(identify().dataFingerprint!=data)throw std::runtime_error("Manifest fingerprint changed");
    // Neither game.db nor a resource directory exists: connection reads only
    // the tiny manifest, independently of the installed data size.
    write("S2-NETWORK-1\n"+std::string(64,'0')+"\n"+data+"\n");
    if(!rejected())throw std::runtime_error("Wrong executable version accepted");
    write("S2-NETWORK-1\n"+build+"\ninvalid/value\n");
    if(!rejected())throw std::runtime_error("Malformed fingerprint accepted");
    write("S2-NETWORK-1\n"+build+"\n"+data+"\nextra\n");
    if(!rejected())throw std::runtime_error("Extra manifest fields accepted");
    write("S2-NETWORK-1\n"+build+"\n"+data+"\n");
    auto overlay=S2Net::IdentifyGameData({}, {(root/"game.db").string(),(root/"game.db").string()},cancelled);
    if(overlay.dataFingerprint!=data+":"+data)throw std::runtime_error("Overlay versions lost their boundaries");
    write("S2-NETWORK-1\n"+build+"\n"+data+"\n");cancelled=true;
    if(!rejected())throw std::runtime_error("Cancellation ignored");
    std::filesystem::remove(root/"network-version.txt");
    std::cout<<"Network identity: small-file read, missing/malformed/build mismatch/cancellation passed\n";
    return 0;
  }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
