#pragma once
#include "iMain.h"
namespace NGame {
class CICNetworkMenu: public NMainLoop::CInterfaceCommand {
  OBJECT_NOCOPY_METHODS(CICNetworkMenu);
  int role;
  int variant;
  unsigned port;
  string ip, status;
 public:
  CICNetworkMenu(int startupRole=0,unsigned startupPort=7780,const string& address="127.0.0.1",const string& message="",int mapVariant=0):
    role(startupRole),variant(mapVariant),port(startupPort),ip(address),status(message) {}
  void Exec();
};
}
