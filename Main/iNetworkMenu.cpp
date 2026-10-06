#include "StdAfx.h"
#include "../MiscDll/Commands.h"
#include "iNetworkMenu.h"
#include "iMainMenu.h"
#include "iMission.h"
#include "iMissionInternal.h"
#include "iCommonUI.h"
#include "Interface.h"
#include "GResource.h"
#include "GView.h"
#include "NetworkWorld.h"
#include "NetworkIdentity.h"
#include "ModManager.h"
#include "G2DView.h"
#include "GfxRender.h"
#include "GfxUtils.h"
#include "VectorFontRasterizer.h"
#include <SDL3/SDL_filesystem.h>
#include "DisplayLayout.h"
#include <future>
#include <atomic>
#include <stdexcept>

namespace NGame {
namespace {
bool ValidAddress(const string& value) {
  unsigned part=0,digits=0,count=0;
  for(char c:value+".") {
    if(c=='.'){if(!digits || part>255)return false;++count;part=digits=0;}
    else {if(c<'0'||c>'9'||++digits>3)return false;part=part*10+c-'0';}
  }
  return count==4;
}
unsigned ValidPort(const wstring& value) {
  if(value.empty() || value.size()>5)return 0;
  unsigned port=0;for(wchar_t c:value){if(c<L'0'||c>L'9')return 0;port=port*10+c-L'0';}
  return port && port<=65535 ? port : 0;
}
wstring NetworkStatusText(const string& message) {
  if(message=="Match already has two players")return L"\u041a \u043c\u0430\u0442\u0447\u0443 \u0443\u0436\u0435 \u043f\u043e\u0434\u043a\u043b\u044e\u0447\u0435\u043d\u044b \u0434\u0432\u0430 \u0438\u0433\u0440\u043e\u043a\u0430.";
  if(message=="Checking game data...")return L"\u041f\u0440\u043e\u0432\u0435\u0440\u043a\u0430 \u0438\u0433\u0440\u043e\u0432\u044b\u0445 \u0434\u0430\u043d\u043d\u044b\u0445...";
  if(message=="Waiting for the second player")return L"\u041e\u0436\u0438\u0434\u0430\u043d\u0438\u0435 \u0432\u0442\u043e\u0440\u043e\u0433\u043e \u0438\u0433\u0440\u043e\u043a\u0430. \u00ab\u041d\u0430\u0437\u0430\u0434\u00bb \u2014 \u043e\u0442\u043c\u0435\u043d\u0430.";
  if(message=="Connecting")return L"\u041f\u043e\u0434\u043a\u043b\u044e\u0447\u0435\u043d\u0438\u0435...";
  if(message=="Loading the match")return L"\u0417\u0430\u0433\u0440\u0443\u0437\u043a\u0430 \u043c\u0430\u0442\u0447\u0430...";
  if(message=="Receiving the match")return L"\u041f\u043e\u043b\u0443\u0447\u0435\u043d\u0438\u0435 \u0441\u043e\u0441\u0442\u043e\u044f\u043d\u0438\u044f \u043c\u0430\u0442\u0447\u0430...";
  if(message=="Connected")return L"\u041f\u043e\u0434\u043a\u043b\u044e\u0447\u0435\u043d\u043e";
  if(message=="Player left")return L"\u0423\u0447\u0430\u0441\u0442\u043d\u0438\u043a \u0432\u044b\u0448\u0435\u043b \u0438\u0437 \u043c\u0430\u0442\u0447\u0430.";
  if(message=="Other player disconnected")return L"\u0421\u043e\u0435\u0434\u0438\u043d\u0435\u043d\u0438\u0435 \u0441 \u0434\u0440\u0443\u0433\u0438\u043c \u0438\u0433\u0440\u043e\u043a\u043e\u043c \u043f\u043e\u0442\u0435\u0440\u044f\u043d\u043e.";
  if(message=="Cancelled")return L"\u041e\u043f\u0435\u0440\u0430\u0446\u0438\u044f \u043e\u0442\u043c\u0435\u043d\u0435\u043d\u0430.";
  if(message=="Draw")return L"\u041c\u0430\u0442\u0447 \u0437\u0430\u0432\u0435\u0440\u0448\u0451\u043d: \u043d\u0438\u0447\u044c\u044f.";
  if(message=="Host won")return L"\u041c\u0430\u0442\u0447 \u0437\u0430\u0432\u0435\u0440\u0448\u0451\u043d: \u043f\u043e\u0431\u0435\u0434\u0438\u043b \u0445\u043e\u0441\u0442.";
  if(message=="Client won")return L"\u041c\u0430\u0442\u0447 \u0437\u0430\u0432\u0435\u0440\u0448\u0451\u043d: \u043f\u043e\u0431\u0435\u0434\u0438\u043b \u043a\u043b\u0438\u0435\u043d\u0442.";
  if(message=="Different game builds")return L"\u0412\u0435\u0440\u0441\u0438\u0438 \u0438\u0433\u0440\u044b \u0440\u0430\u0437\u043b\u0438\u0447\u0430\u044e\u0442\u0441\u044f.";
  if(message=="Different effective game data")return L"\u0418\u0433\u0440\u043e\u0432\u044b\u0435 \u0434\u0430\u043d\u043d\u044b\u0435 \u0440\u0430\u0437\u043b\u0438\u0447\u0430\u044e\u0442\u0441\u044f.";
  if(message=="Incompatible network protocol")return L"\u041d\u0435\u0441\u043e\u0432\u043c\u0435\u0441\u0442\u0438\u043c\u0430\u044f \u0432\u0435\u0440\u0441\u0438\u044f \u0441\u0435\u0442\u0435\u0432\u043e\u0433\u043e \u043f\u0440\u043e\u0442\u043e\u043a\u043e\u043b\u0430.";
  if(message=="Host compatibility mismatch")return L"\u0412\u0435\u0440\u0441\u0438\u044f \u0438\u043b\u0438 \u0438\u0433\u0440\u043e\u0432\u044b\u0435 \u0434\u0430\u043d\u043d\u044b\u0435 \u0445\u043e\u0441\u0442\u0430 \u043d\u0435 \u0441\u043e\u0432\u043f\u0430\u0434\u0430\u044e\u0442.";
  if(message=="Connection refused")return L"\u041f\u043e\u0434\u043a\u043b\u044e\u0447\u0438\u0442\u044c\u0441\u044f \u043d\u0435 \u0443\u0434\u0430\u043b\u043e\u0441\u044c. \u041f\u0440\u043e\u0432\u0435\u0440\u044c\u0442\u0435 IP \u0438 \u043f\u043e\u0440\u0442.";
  if(message=="Connection timed out")return L"\u0418\u0441\u0442\u0435\u043a\u043b\u043e \u0432\u0440\u0435\u043c\u044f \u043f\u043e\u0434\u043a\u043b\u044e\u0447\u0435\u043d\u0438\u044f. \u041f\u0440\u043e\u0432\u0435\u0440\u044c\u0442\u0435 IP \u0438 \u043f\u043e\u0440\u0442.";
  if(message=="Timed out waiting for the other player")return L"\u0414\u0440\u0443\u0433\u043e\u0439 \u0438\u0433\u0440\u043e\u043a \u043d\u0435 \u043e\u0442\u0432\u0435\u0442\u0438\u043b \u0432\u043e\u0432\u0440\u0435\u043c\u044f.";
  if(message=="Cannot listen: port is unavailable")return L"\u041f\u043e\u0440\u0442 \u0437\u0430\u043d\u044f\u0442 \u0438\u043b\u0438 \u043d\u0435\u0434\u043e\u0441\u0442\u0443\u043f\u0435\u043d. \u0412\u044b\u0431\u0435\u0440\u0438\u0442\u0435 \u0434\u0440\u0443\u0433\u043e\u0439 \u043f\u043e\u0440\u0442.";
  if(message=="Enter a numeric IPv4 address and port 1..65535")return L"\u0412\u0432\u0435\u0434\u0438\u0442\u0435 \u0447\u0438\u0441\u043b\u043e\u0432\u043e\u0439 IPv4 \u0438 \u043f\u043e\u0440\u0442 \u043e\u0442 1 \u0434\u043e 65535.";
  if(message=="Port must be between 1 and 65535")return L"\u041f\u043e\u0440\u0442 \u0434\u043e\u043b\u0436\u0435\u043d \u0431\u044b\u0442\u044c \u043e\u0442 1 \u0434\u043e 65535.";
  if(message=="Invalid port")return L"\u041d\u0435\u043a\u043e\u0440\u0440\u0435\u043a\u0442\u043d\u044b\u0439 \u043f\u043e\u0440\u0442.";
  if(message=="No random encounter maps available")return L"\u0412 \u0438\u0433\u0440\u043e\u0432\u044b\u0445 \u0434\u0430\u043d\u043d\u044b\u0445 \u043d\u0435\u0442 \u043a\u0430\u0440\u0442 \u0441\u043b\u0443\u0447\u0430\u0439\u043d\u044b\u0445 \u0441\u0442\u043e\u043b\u043a\u043d\u043e\u0432\u0435\u043d\u0438\u0439.";
  if(message=="Selected map is not a random encounter")return L"\u0412\u044b\u0431\u0440\u0430\u043d\u043d\u0430\u044f \u043a\u0430\u0440\u0442\u0430 \u043d\u0435 \u044f\u0432\u043b\u044f\u0435\u0442\u0441\u044f \u0441\u043b\u0443\u0447\u0430\u0439\u043d\u044b\u043c \u0441\u0442\u043e\u043b\u043a\u043d\u043e\u0432\u0435\u043d\u0438\u0435\u043c.";
  auto systemCode=message.rfind(" (OS ");
  if(systemCode!=std::string::npos && message.back()==')') {
    auto base=message.substr(0,systemCode), suffix=message.substr(systemCode);
    auto translated=NetworkStatusText(base);
    return translated+wstring(suffix.begin(),suffix.end());
  }
  if(message=="Send failed")return L"\u041e\u0448\u0438\u0431\u043a\u0430 \u043e\u0442\u043f\u0440\u0430\u0432\u043a\u0438 \u0434\u0430\u043d\u043d\u044b\u0445. \u041c\u0430\u0442\u0447 \u0437\u0430\u0432\u0435\u0440\u0448\u0451\u043d.";
  if(message=="Receive failed")return L"\u041e\u0448\u0438\u0431\u043a\u0430 \u043f\u043e\u043b\u0443\u0447\u0435\u043d\u0438\u044f \u0434\u0430\u043d\u043d\u044b\u0445. \u041c\u0430\u0442\u0447 \u0437\u0430\u0432\u0435\u0440\u0448\u0451\u043d.";
  if(message=="Connection closed")return L"\u0421\u043e\u0435\u0434\u0438\u043d\u0435\u043d\u0438\u0435 \u0437\u0430\u043a\u0440\u044b\u0442\u043e.";
  if(message=="Network send queue overflow")return L"\u0414\u0440\u0443\u0433\u043e\u0439 \u0438\u0433\u0440\u043e\u043a \u043d\u0435 \u0443\u0441\u043f\u0435\u0432\u0430\u0435\u0442 \u043f\u043e\u043b\u0443\u0447\u0430\u0442\u044c \u0441\u043e\u0441\u0442\u043e\u044f\u043d\u0438\u0435. \u041c\u0430\u0442\u0447 \u0437\u0430\u0432\u0435\u0440\u0448\u0451\u043d.";
  return wstring(message.begin(),message.end());
}
const wchar_t* Font=L"<font face=System size=20pt color=white>";
const wchar_t* Caption=L"\u0421\u0435\u0442\u0435\u0432\u0430\u044f \u0438\u0433\u0440\u0430";
class NetworkControls:public NUI::CWindow {
  OBJECT_NOCOPY_METHODS(NetworkControls);
  string displayedStatus;
  S2Fonts::Atlas fontAtlas;
  int fontPixels=0;
  CObj<NGfx::CTexture> canvas;
  wstring canvasKey;
  vector<S2Net::NetworkMapChoice> maps;
  size_t selectedMap=0;
  bool mapList=false;
  size_t mapPage=0;
  enum { MapsPerPage=16 };
 public:
  NetworkControls() {}
  CMObj<NUI::CEdit> address,port;
  CMObj<NUI::CText> status,mapName;
  CMObj<NUI::CHoverButton> create,connect,back,previousMap,nextMap;
  CMObj<NUI::CHoverButton> mapPicker,mapRows[MapsPerPage],previousPage,nextPage,closeList;
  std::function<void(bool)> start;
  std::function<void()> cancel;
  explicit NetworkControls(const NUI::SWindowInfo& info):CWindow(info) {
    using namespace NUI;
    auto label=[this](const char* id,int x,int y,int width,const wstring& value) {
      auto* text=new CText(SWindowInfo(this,SPoint(x,y),SPoint(width,50),id));
      text->SetText(wstring(Font)+value);return text;
    };
    label("title",260,150,500,Caption);
    label("ip_label",260,235,180,L"IP (IPv4)");label("port_label",260,300,180,L"\u041f\u043e\u0440\u0442");
    address=new CEdit(SWindowInfo(this,SPoint(465,235),SPoint(300,50),"net_address"));
    address->SetTextFormat(Font);address->SetEditSize(15);address->SetText(NGlobal::GetVar("net_address",wstring(L"127.0.0.1")).GetString());
    port=new CEdit(SWindowInfo(this,SPoint(465,300),SPoint(160,50),"net_port"));
    port->SetTextFormat(Font);port->SetMode(CEdit::NUMERIC);port->SetEditSize(5);port->SetText(std::to_wstring(NGlobal::GetVar("net_port",7780).GetInt()));
    auto button=[this](const char* id,int x,int y,int width,const wstring& caption) {
      auto* b=new CHoverButton(SWindowInfo(this,SPoint(x,y),SPoint(width,60),id));
      b->AddTextState(CHoverButton::STATE_NORMAL,wstring(Font)+caption);
      b->AddTextState(CHoverButton::STATE_HOVER,L"<font face=System size=20pt color=yellow>"+caption);
      b->AddTextState(CHoverButton::STATE_DISABLED,L"<font face=System size=20pt color=gray>"+caption);return b;
    };
    label("map_label",260,365,520,L"\u041a\u0430\u0440\u0442\u0430 \u043f\u0440\u0438 \u0441\u043e\u0437\u0434\u0430\u043d\u0438\u0438 \u043c\u0430\u0442\u0447\u0430");
    previousMap=button("net_map_previous",145,410,60,L"<");
    nextMap=button("net_map_next",825,410,60,L">");
    mapName=label("net_map",230,425,565,L"");
    mapPicker=button("net_map_list",230,410,565,L"");
    maps=S2Net::GetNetworkEncounterMaps();UpdateMapName();
    create=button("net_create",145,505,230,L"\u0421\u043e\u0437\u0434\u0430\u0442\u044c");
    connect=button("net_connect",395,505,230,L"\u041f\u043e\u0434\u043a\u043b\u044e\u0447\u0438\u0442\u044c\u0441\u044f");
    back=button("net_back",645,505,230,L"\u041d\u0430\u0437\u0430\u0434");
    status=label("net_status",145,605,740,L"");
    for(int row=0;row<MapsPerPage;++row) {
      mapRows[row]=button(("net_map_pick_"+std::to_string(row)).c_str(),145,210+26*row,740,L"");
      mapRows[row]->SetSize(SPoint(740,26));
    }
    previousPage=button("net_map_page_previous",145,660,230,L"<");
    closeList=button("net_map_close",395,660,230,L"\u0417\u0430\u043a\u0440\u044b\u0442\u044c");
    nextPage=button("net_map_page_next",645,660,230,L">");
    ShowMapList(false);
  }
  void ShowMapList(bool open) {
    mapList=open;
    for(const char* id:{"ip_label","port_label","net_address","net_port","map_label","net_map","net_map_list","net_map_previous","net_map_next","net_create","net_connect","net_back","net_status"})
      GetChildByID(id)->ShowWindow(open?NUI::SWTYPE_HIDE:NUI::SWTYPE_SHOW);
    for(int row=0;row<MapsPerPage;++row)mapRows[row]->ShowWindow(open && mapPage*MapsPerPage+row<maps.size()?NUI::SWTYPE_SHOW:NUI::SWTYPE_HIDE);
    for(auto* control:{previousPage.GetPtr(),nextPage.GetPtr(),closeList.GetPtr()})control->ShowWindow(open?NUI::SWTYPE_SHOW:NUI::SWTYPE_HIDE);
  }
  void UpdateMapName() {mapName->SetText(wstring(Font)+MapCaption());}
  wstring MapCaption() const {return maps.empty()?L"\u041d\u0435\u0442 \u043a\u0430\u0440\u0442 \u0441\u043b\u0443\u0447\u0430\u0439\u043d\u044b\u0445 \u0441\u0442\u043e\u043b\u043a\u043d\u043e\u0432\u0435\u043d\u0438\u0439":maps[selectedMap].name+L" ("+std::to_wstring(selectedMap+1)+L"/"+std::to_wstring(maps.size())+L")";}
  int SelectedVariant() const {return maps.empty()?0:maps[selectedMap].variantID;}
  bool SelectVariant(int id) {
    for(size_t i=0;i<maps.size();++i)if(maps[i].variantID==id){selectedMap=i;UpdateMapName();return true;}
    return false;
  }
  bool ProcessMessage(const NUI::SEvent& event) {
    if(event.nEvent==NUI::EVENT_NOTIFY) {
      if(event.szID=="net_map_list" && !maps.empty()){mapPage=selectedMap/MapsPerPage;ShowMapList(true);return true;}
      if(event.szID=="net_map_close"){ShowMapList(false);return true;}
      if(mapList) {
        if(event.szID=="net_map_page_previous" || event.szID=="net_map_page_next") {
          const size_t pages=(maps.size()+MapsPerPage-1)/MapsPerPage;
          mapPage=(event.szID=="net_map_page_next"?mapPage+1:mapPage+pages-1)%pages;ShowMapList(true);return true;
        }
        for(int row=0;row<MapsPerPage;++row)if(event.szID=="net_map_pick_"+std::to_string(row)) {
          size_t index=mapPage*MapsPerPage+row;if(index<maps.size()){selectedMap=index;UpdateMapName();ShowMapList(false);}return true;
        }
      }
      if((event.szID=="net_map_previous" || event.szID=="net_map_next") && !maps.empty()) {
        selectedMap=(event.szID=="net_map_next"?selectedMap+1:selectedMap+maps.size()-1)%maps.size();UpdateMapName();return true;
      }
      if(event.szID=="net_create" && start){start(true);return true;}
      if(event.szID=="net_connect" && start){start(false);return true;}
      if(event.szID=="net_back" && cancel){cancel();return true;}
    }
    return CWindow::ProcessMessage(event);
  }
  void Draw(const STime& time,NGScene::I2DGameView* view) override {
    // Native widgets retain editing, focus, hit testing and button behavior.
    // Composite this small form into one texture so changing status text cannot
    // invalidate earlier glyph batches in the legacy presentation pools.
    CWindow::Draw(time,view);view->Flush();
    const auto size=view->GetViewportSize();const int width=Float2Int(size.x),height=Float2Int(size.y);
    const int pixels=Max(1,Float2Int(20*S2UI::Scale()));
    if(fontPixels!=pixels) {
      string error;const char* base=SDL_GetBasePath();
      if(!base || !S2Fonts::Rasterize(string(base)+"fonts/"+S2Fonts::FamilyFile("System"),pixels,&fontAtlas,&error))return;
      fontPixels=pixels;canvasKey.clear();
    }
    wstring key=address->GetText()+L"|"+port->GetText()+L"|"+MapCaption()+L"|"+NetworkStatusText(displayedStatus);
    key+=std::to_wstring(width)+L","+std::to_wstring(height);
    key+=(mapList?L"list":L"form")+std::to_wstring(mapPage);
    for(auto* control:{create.GetPtr(),connect.GetPtr(),back.GetPtr(),previousMap.GetPtr(),nextMap.GetPtr(),mapPicker.GetPtr(),previousPage.GetPtr(),nextPage.GetPtr(),closeList.GetPtr()}) {
      key+=control->IsMouseCover()?L"h":L"-";
      key+=control->GetStyle(NUI::STYLE_ENABLED)?L"e":L"d";
    }
    if(mapList)for(auto& row:mapRows)key+=row->IsMouseCover()?L"h":L"-";
    const bool blink=(S2Platform::Milliseconds()/500)%2==0;
    key+=address->IsActive()?L"a":port->IsActive()?L"p":L"-";
    key+=std::to_wstring(address->GetCursorPosition())+L","+std::to_wstring(port->GetCursorPosition());
    if(address->IsActive() || port->IsActive())key+=blink?L"1":L"0";
    if(key!=canvasKey || !canvas) {
      if(!canvas || canvas->GetXSize()!=width || canvas->GetYSize()!=height)
        canvas=NGfx::MakeTexture(width,height,1,NGfx::SPixel8888::ID,NGfx::REGULAR,NGfx::CLAMP);
      NGfx::CTextureLock<NGfx::SPixel8888> lock(canvas,0,NGfx::INPLACE);
      for(int y=0;y<height;++y)for(int x=0;x<width;++x)lock[y][x]=NGfx::SPixel8888(16,16,24,255);
      auto text=[&](NUI::CWindow* window,const wstring& value,int offset,NGfx::SPixel8888 color,int caret=-1) {
        NUI::SPoint position;NUI::SRect clip;if(!window->ClientToScreen(&position,&clip))return;
        window->VirtualToScreen(&position,&clip);
        int x=position.x,y=position.y+Float2Int(offset*S2UI::Scale()),index=0;WORD previous=0;
        auto cursor=[&] {if(blink)for(int cy=y;cy<y+fontAtlas.lineHeight && cy<clip.y2 && cy<height;++cy)
          if(x>=0 && x<width && cy>=0)lock[cy][x]=color;};
        for(wchar_t letter:value) {
          if(index++==caret)cursor();
          const S2Fonts::Glyph* glyph=nullptr;for(const auto& g:fontAtlas.glyphs)if(g.code==WORD(letter)){glyph=&g;break;}
          if(!glyph)continue;
          int kern=0;for(const auto& k:fontAtlas.kerns)if(k.previous==previous && k.current==WORD(letter)){kern=k.adjustment;break;}
          x+=kern;
          if(letter==L'\n' || x+glyph->advance>clip.x2){x=position.x;y+=fontAtlas.lineHeight;previous=0;if(letter==L'\n')continue;}
          for(int gy=0;gy<glyph->height;++gy)for(int gx=0;gx<glyph->width;++gx) {
            const int dx=x+glyph->bearing+gx,dy=y+gy;
            if(dx<clip.x1 || dx>=clip.x2 || dy<clip.y1 || dy>=clip.y2 || dx<0 || dx>=width || dy<0 || dy>=height)continue;
            const unsigned alpha=fontAtlas.rgba[((glyph->y+gy)*fontAtlas.width+glyph->x+gx)*4+3];
            auto& pixel=lock[dy][dx];pixel.r=(color.r*alpha+pixel.r*(255-alpha)+127)/255;
            pixel.g=(color.g*alpha+pixel.g*(255-alpha)+127)/255;pixel.b=(color.b*alpha+pixel.b*(255-alpha)+127)/255;
          }
          x+=glyph->advance;previous=WORD(letter);
        }
        if(index==caret)cursor();
      };
      const NGfx::SPixel8888 white(255,255,255,255),gray(150,150,150,255),yellow(255,255,0,255);
      if(mapList) {
        text(GetChildByID("title"),L"\u0412\u044b\u0431\u043e\u0440 \u043a\u0430\u0440\u0442\u044b ("+std::to_wstring(mapPage+1)+L"/"+std::to_wstring((maps.size()+MapsPerPage-1)/MapsPerPage)+L")",0,white);
        for(int row=0;row<MapsPerPage;++row) {
          const size_t index=mapPage*MapsPerPage+row;if(index>=maps.size())break;
          text(mapRows[row],maps[index].name,0,index==selectedMap || mapRows[row]->IsMouseCover()?yellow:white);
        }
        text(previousPage,L"<",18,previousPage->IsMouseCover()?yellow:white);
        text(nextPage,L">",18,nextPage->IsMouseCover()?yellow:white);
        text(closeList,L"\u0417\u0430\u043a\u0440\u044b\u0442\u044c",18,closeList->IsMouseCover()?yellow:white);
      } else {
      text(GetChildByID("title"),Caption,0,white);text(GetChildByID("ip_label"),L"IP (IPv4)",0,white);
      text(GetChildByID("port_label"),L"\u041f\u043e\u0440\u0442",0,white);
      text(address,address->GetText(),0,white,address->IsActive()?address->GetCursorPosition():-1);
      text(port,port->GetText(),0,white,port->IsActive()?port->GetCursorPosition():-1);
      text(GetChildByID("map_label"),L"\u041a\u0430\u0440\u0442\u0430 \u043f\u0440\u0438 \u0441\u043e\u0437\u0434\u0430\u043d\u0438\u0438 \u043c\u0430\u0442\u0447\u0430",0,white);
      text(mapName,MapCaption(),0,mapPicker->IsMouseCover()?yellow:white);
      auto button=[&](NUI::CHoverButton* control,const wchar_t* caption) {text(control,caption,18,
          !control->GetStyle(NUI::STYLE_ENABLED)?gray:control->IsMouseCover()?yellow:white);};
      button(create,L"\u0421\u043e\u0437\u0434\u0430\u0442\u044c");
      button(connect,L"\u041f\u043e\u0434\u043a\u043b\u044e\u0447\u0438\u0442\u044c\u0441\u044f");
      button(back,L"\u041d\u0430\u0437\u0430\u0434");text(status,NetworkStatusText(displayedStatus),0,white);
      button(previousMap,L"<");button(nextMap,L">");
      }
      canvasKey=key;
    }
    NGfx::CRenderContext context;context.SetDepth(NGfx::DEPTH_NONE);context.SetCulling(NGfx::CULL_NONE);
    context.SetAlphaCombine(NGfx::COMBINE_NONE);
    NGfx::C2DQuadsRenderer renderer(context,size,NGfx::QRM_NOCOLOR|NGfx::QRM_DEPTH_NONE);
    renderer.AddRect(CTRect<float>(0,0,width,height),canvas,CTRect<float>(0,0,width,height),NGfx::SPixel8888(255,255,255,255));
    renderer.Flush();
  }
  void SetStatus(const string& message) {
    if(displayedStatus==message)return;
    displayedStatus=message;status->SetText(wstring(Font)+NetworkStatusText(message),false);
  }
  void RememberConnection() {
    auto text=address->GetText();string ip(text.begin(),text.end());
    if(ValidAddress(ip))NGlobal::SetVar("net_address",text);
    auto number=ValidPort(port->GetText());if(number)NGlobal::SetVar("net_port",int(number));
    NGlobal::SaveConfig(".\\cfg\\config.cfg");
  }
  void Enable(bool enabled){create->SetStyle(NUI::STYLE_ENABLED,enabled && !maps.empty());connect->SetStyle(NUI::STYLE_ENABLED,enabled);address->SetStyle(NUI::STYLE_ENABLED,enabled);port->SetStyle(NUI::STYLE_ENABLED,enabled);
    previousMap->SetStyle(NUI::STYLE_ENABLED,enabled && maps.size()>1);nextMap->SetStyle(NUI::STYLE_ENABLED,enabled && maps.size()>1);
    mapPicker->SetStyle(NUI::STYLE_ENABLED,enabled && !maps.empty());}
};
class NetworkMission:public NMainLoop::CInterfaceCommand {
  OBJECT_NOCOPY_METHODS(NetworkMission);
  std::shared_ptr<S2Net::NetworkWorld> world;
  std::shared_ptr<S2Net::NetworkSession> session;
 public:
  NetworkMission() {}
  NetworkMission(std::shared_ptr<S2Net::NetworkWorld> w,std::shared_ptr<S2Net::NetworkSession> s):world(std::move(w)),session(std::move(s)){}
  void Exec() {
    try {
      ResetStack();
      CObj<CMission> mission=new CMission;
      if(!mission->InitializeNetwork(world,session))throw std::runtime_error("Cannot initialize the network mission");
      SetInterface(mission);
    }catch(const SFileIOError& e){session->Finish(e.szError);NMainLoop::Command(new CICNetworkMenu(0,7780,"127.0.0.1",e.szError));}
     catch(const std::exception& e){session->Finish(e.what());NMainLoop::Command(new CICNetworkMenu(0,7780,"127.0.0.1",e.what()));}
  }
};
class NetworkScreen:public CMissionBase {
  OBJECT_NOCOPY_METHODS(NetworkScreen);
  CMObj<NetworkControls> controls;
  std::shared_ptr<S2Net::NetworkWorld> world;
  std::shared_ptr<S2Net::NetworkSession> session;
  std::shared_ptr<std::atomic<bool>> cancelled;
  std::future<S2Net::Compatibility> identity;
  bool preparing=false,host=false,opening=false,cancelPending=false;
  unsigned selectedPort=7780;
  string selectedAddress;
  S2Net::NetworkMatchPreset preset;
 public:
  void SetStatus(const string& message) {controls->SetStatus(message);}
  void BuildControls(const string& message) {
    controls=new NetworkControls(NUI::SWindowInfo(GetInterface(),NUI::SPoint(0,0),NUI::SPoint(1024,768),"network_menu"));
    controls->start=[this](bool h){Start(h);};
    controls->cancel=[this]{controls->RememberConnection();if(cancelled)cancelled->store(true);if(session)session->Finish("Cancelled");
      if(session && session->State()==S2Net::SessionState::Finishing){cancelPending=true;controls->Enable(false);}
      else NMainLoop::Command(new CICMainMenu);};
    controls->SetStatus(message);controls->ShowWindow(NUI::SWTYPE_SHOW);
  }
  void Initialize(const string& message) {
    bCanSave=false;bCanRestart=false;
    pCursor=NUI::ICursor::Create();pInterface=new NUI::CInterface(pCursor,0);
    BuildControls(message);
  }
  void Start(bool authoritative) {
    if(preparing || opening)return;
    try {
      auto value=controls->port->GetText();
      if(value.empty() || value.size()>5)throw std::runtime_error("Port must be between 1 and 65535");
      selectedPort=0;for(auto c:value){if(c<L'0'||c>L'9')throw std::runtime_error("Invalid port");selectedPort=selectedPort*10+(c-L'0');}
      if(!selectedPort || selectedPort>65535)throw std::runtime_error("Port must be between 1 and 65535");
      auto address=controls->address->GetText();selectedAddress=string(address.begin(),address.end());
      if(!authoritative && !ValidAddress(selectedAddress))throw std::runtime_error("Enter a numeric IPv4 address and port 1..65535");
      controls->RememberConnection();
      host=authoritative;world.reset();session.reset();
      if(host) {
        preset.mapVariant=controls->SelectedVariant();preset.useEncounterDeployment=true;
        if(!preset.mapVariant)throw std::runtime_error("No random encounter maps available");
      }
      SetStatus("Checking game data...");controls->Enable(false);
      auto dirs=NGScene::GetNetworkResourceDirectories();
      vector<string> databases={"game.db"};
      for(const auto& mod:*CModManager::GetActiveMods())databases.push_back(mod.szDirectory+"/game.db");
      cancelled=std::make_shared<std::atomic<bool>>(false);auto stop=cancelled;
      identity=std::async(std::launch::async,[dirs,databases,stop]{return S2Net::IdentifyGameData(dirs,databases,*stop);});
      preparing=true;
    }catch(const std::exception& e){SetStatus(e.what());controls->Enable(true);}
  }
  void Autostart(int role,unsigned port,const string& address,int variant) {
    if(role){controls->port->SetText(std::to_wstring(port));controls->address->SetText(wstring(address.begin(),address.end()));}
    if(variant && !controls->SelectVariant(variant)){SetStatus("Selected map is not a random encounter");return;}
    if(role)Start(role==1);
  }
  void Step() {
    try {
      if(preparing && identity.wait_for(std::chrono::seconds(0))==std::future_status::ready) {
        auto fingerprint=identity.get();preparing=false;
        world=std::make_shared<S2Net::NetworkWorld>(host,preset);
        session=std::make_shared<S2Net::NetworkSession>(fingerprint,world->Callbacks());
        if(host)session->Host(static_cast<std::uint16_t>(selectedPort));
        else session->Connect(selectedAddress,static_cast<std::uint16_t>(selectedPort));
      }
      if(session) {
        session->Poll();SetStatus(session->Status());controls->Enable(false);
        if(cancelPending && session->State()!=S2Net::SessionState::Finishing){NMainLoop::Command(new CICMainMenu);return;}
        if(session->State()==S2Net::SessionState::Failed || session->State()==S2Net::SessionState::Finished)controls->Enable(true);
        if(!opening && session->State()==S2Net::SessionState::Playing) {opening=true;NMainLoop::Command(new NetworkMission(world,session));return;}
      }
    }catch(const SFileIOError& e){preparing=false;SetStatus(e.szError);controls->Enable(true);}
     catch(const std::exception& e){preparing=false;SetStatus(e.what());controls->Enable(true);}
    controls->Enable(!preparing && (!session || session->State()==S2Net::SessionState::Failed || session->State()==S2Net::SessionState::Finished));
    if(CanRender()) {
      pInterface->UpdateCursor();pInterface->Step(GetTime());
      MarkNewDGFrame();
      NGfx::CRenderContext context;context.ClearBuffers(0xff101018);
      GetInterface()->Draw(GetTime());NGScene::Flip();
    }
  }
  bool RenderWhenInactive() const override { return true; }
  void OnGetFocus() override {}
  void OnLostFocus() override {}
  bool ProcessEvent(const NInput::SEvent& event) override {
    pCursor->ProcessEvent(event);return pInterface->ProcessEvent(event);
  }
 protected:
  ~NetworkScreen(){if(cancelled)cancelled->store(true);}
};
}
void CICNetworkMenu::Exec() {
  ResetStack();
  CObj<NetworkScreen> screen=new NetworkScreen;screen->Initialize(status);screen->Autostart(role,port,ip,variant);
  SetInterface(screen);
}
START_REGISTER(NetworkMenu)
  REGISTER_VAR("net_address",0,wstring(L"127.0.0.1"),true)
  REGISTER_VAR("net_port",0,7780,true)
FINISH_REGISTER
}
