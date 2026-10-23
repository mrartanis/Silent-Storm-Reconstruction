#pragma once
#include "iMain.h"
#include <memory>
namespace NUI {class CWindow;}
namespace S2Net {class NetworkWorld;class NetworkSession;}
namespace NGame {
class CMission;
void OpenNetworkMission(std::shared_ptr<S2Net::NetworkWorld>,std::shared_ptr<S2Net::NetworkSession>);
CObj<NUI::CWindow> CreateNetworkPreparationControls(CMission*,std::shared_ptr<S2Net::NetworkWorld>,std::shared_ptr<S2Net::NetworkSession>);
CObj<NUI::CWindow> CreateNetworkMatchMenu(CMission*,std::shared_ptr<S2Net::NetworkSession>);
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
