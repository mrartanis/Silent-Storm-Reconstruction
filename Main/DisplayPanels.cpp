#include "StdAfx.h"
#include "Interface.h"
#include "DisplayLayout.h"
#include "DisplayOptions.h"
#include "GInit.h"
#include "../MiscDll/Commands.h"

namespace NUI {
namespace {
bool RussianDisplayText() {
  for(wchar_t c:GetDBString(4382)) if(c>=0x400 && c<=0x4ff) return true;
  return false;
}
CWindow* Panel(CInterface* ui, const char* id, int height) {
  CWindow* old = ui->GetChildByID(id);
  if (IsValid(old)) { old->ShowWindow(SWTYPE_SHOW); return old; }
  auto* panel = new CWindow(SWindowInfo(ui, SPoint(240, (768-height)/2), SPoint(544,height), id,
      STYLE_ENABLED | STYLE_VISIBLE | STYLE_TOPMOST | STYLE_MODAL));
  panel->EnableAdaptiveLayout();
  auto* background = new CImage(SWindowInfo(panel, SPoint(0,0), SPoint(544,height), "background",
      STYLE_ENABLED | STYLE_VISIBLE | STYLE_TRANSPARENT | STYLE_BOTTOMMOST));
  const bool confirmation=std::string(id)=="display_confirmation";
  background->SetColor(confirmation?NGfx::SPixel8888(216,203,175,255):NGfx::SPixel8888(12,16,20,245));
  if(confirmation) {
    const SPoint positions[]={SPoint(0,0),SPoint(0,height-3),SPoint(0,3),SPoint(541,3)};
    const SPoint sizes[]={SPoint(544,3),SPoint(544,3),SPoint(3,height-6),SPoint(3,height-6)};
    for(int i=0;i<4;++i) {
      auto* border=new CImage(SWindowInfo(panel,positions[i],sizes[i],"border",STYLE_ENABLED|STYLE_VISIBLE|STYLE_TRANSPARENT));
      border->SetColor(NGfx::SPixel8888(89,58,39,255));
    }
  }
  panel->ShowWindow(SWTYPE_SHOW);
  return panel;
}
void Label(CWindow* parent, const char* id, int y, const wstring& text) {
  CText* label = dynamic_cast<CText*>(parent->GetChildByID(id));
  if (!IsValid(label)) label = new CText(SWindowInfo(parent, SPoint(16,y), SPoint(512,id==std::string("countdown")?64:32),id,
      STYLE_VISIBLE | STYLE_ENABLED | STYLE_TRANSPARENT));
  if (label->GetText()!=text) label->SetText(text);
}
void Button(CWindow* parent, const char* id, int y, const wstring& text) {
  CButton* button = dynamic_cast<CButton*>(parent->GetChildByID(id));
  if (!IsValid(button)) {
    button = new CButton(SWindowInfo(parent,SPoint(16,y),SPoint(512,34),id));
    CText* label = new CText(SWindowInfo(button,SPoint(8,0),SPoint(496,34),"caption",
        STYLE_VISIBLE | STYLE_ENABLED | STYLE_TRANSPARENT));
    label->SetText(text);
  } else {
    CText* label = dynamic_cast<CText*>(button->GetChildByID("caption"));
    if (IsValid(label) && label->GetText()!=text) label->SetText(text);
  }
}
void NativeButton(CWindow* parent, const char* id, int y, const wstring& text) {
  if(IsValid(parent->GetChildByID(id))) return;
  auto* button=new CButton(SWindowInfo(parent,SPoint(16,y),SPoint(512,36),id));
  button->AddTextState(0,GetDBString(4404)+L"<center>"+text);
  button->AddTextState(1,GetDBString(4405)+L"<center>"+text);
  button->AddTextState(2,GetDBString(4405)+L"<center>"+text);
}
}
void CInterface::ShowDisplayOptions() {
  ResetMouseCapture();
  Panel(this,"display_settings",300);
  UpdateDisplayPanels();
}
void CInterface::UpdateDisplayPanels() {
  CWindow* settings=GetChildByID("display_settings");
  if (IsValid(settings) && settings->GetStyle(STYLE_VISIBLE)) {
    BringWindowToTop(settings);
    Label(settings,"title",12,L"<font size=22pt>Display and interface");
    const int mode=NGlobal::GetVar("gfx_fullscreen",1).GetInt();
    Button(settings,"display_mode",58,mode==2?L"<font size=18pt>Window mode: Borderless":mode==1?L"<font size=18pt>Window mode: Fullscreen":L"<font size=18pt>Window mode: Window");
    WCHAR text[256];
    const float percent=NGlobal::GetVar("ui_scale",0).GetFloat();
    const auto& m=S2Platform::Display();
    if (percent<=0) swprintf(text,256,L"<font size=18pt>UI scale: Auto (applied %.0f%%)",m.uiScale/m.displayScale*100);
    else swprintf(text,256,L"<font size=18pt>UI scale: %.0f%% (applied %.0f%%)",percent,m.uiScale/m.displayScale*100);
    Button(settings,"display_scale",102,text);
    Button(settings,"display_fonts",146,NGlobal::GetVar("ui_vector_fonts",1).GetFloat()!=0 ? L"<font size=18pt>Fonts: Vector" : L"<font size=18pt>Fonts: Original bitmap");
    Label(settings,"hint",193,L"<font size=14pt>Click a setting to change it. Scale is limited to fit the UI.");
    Button(settings,"display_close",246,L"<font size=18pt><center>Close");
  }
  const int seconds=NGScene::DisplayChangeSeconds();
  CWindow* confirmation=GetChildByID("display_confirmation");
  if(seconds>0) {
    confirmation=Panel(this,"display_confirmation",220);
    BringWindowToTop(confirmation);
    const bool russian=RussianDisplayText();
    WCHAR text[256]; swprintf(text,256,russian?
      L"\u0421\u043e\u0445\u0440\u0430\u043d\u0438\u0442\u044c \u0432\u044b\u0431\u0440\u0430\u043d\u043d\u044b\u0439 \u0440\u0435\u0436\u0438\u043c?<br>\u0412\u043e\u0437\u0432\u0440\u0430\u0442 \u0447\u0435\u0440\u0435\u0437 %d \u0441\u0435\u043a.":
      L"Keep this display mode?<br>Reverting in %d seconds.",seconds);
    Label(confirmation,"countdown",20,GetDBString(4402)+L"<center>"+text);
    NativeButton(confirmation,"display_keep",92,russian?L"\u0421\u041e\u0425\u0420\u0410\u041d\u0418\u0422\u042c (ENTER)":L"KEEP (ENTER)");
    NativeButton(confirmation,"display_revert",140,russian?L"\u0412\u0415\u0420\u041d\u0423\u0422\u042c (ESCAPE)":L"REVERT (ESCAPE)");
  } else if(IsValid(confirmation)) confirmation->SetStyle(STYLE_VISIBLE,false);
}
bool CInterface::HandleDisplayMessage(const SEvent& event) {
  if(event.nEvent==EVENT_WINKEY) {
    if(NGScene::DisplayChangeSeconds()>0 && (event.nVal==13 || event.nVal==27)) {
      if(event.nVal==13) NGScene::ConfirmDisplayChange(); else NGScene::RevertDisplayChange();
      UpdateDisplayPanels(); return true;
    }
    CWindow* settings=GetChildByID("display_settings");
    if(event.nVal==27 && IsValid(settings) && settings->GetStyle(STYLE_VISIBLE)) {
      settings->SetStyle(STYLE_VISIBLE,false); return true;
    }
  }
  if(event.nEvent!=EVENT_NOTIFY) return false;
  const auto& id=event.szID;
  if(id=="display_keep") NGScene::ConfirmDisplayChange();
  else if(id=="display_revert") NGScene::RevertDisplayChange();
  else if(id=="display_close") { CWindow* panel=GetChildByID("display_settings"); if(IsValid(panel)) panel->SetStyle(STYLE_VISIBLE,false); }
  else if(id=="display_mode") {
    NGScene::BeginDisplayChange();
    NGlobal::SetVar("gfx_fullscreen",(NGlobal::GetVar("gfx_fullscreen",1).GetInt()+1)%3);
    if(!NGScene::SetModeFromConfig(false)) NGScene::RevertDisplayChange();
  } else if(id=="display_scale") {
    const int choices[]={0,75,100,125,150,200};
    const int old=static_cast<int>(NGlobal::GetVar("ui_scale",0).GetFloat());
    int next=0; for(int i=0;i<6;++i) if(choices[i]==old) next=choices[(i+1)%6];
    NGlobal::SetVar("ui_scale",static_cast<float>(next));
    S2Platform::UpdateDisplay(static_cast<float>(next));
  } else if(id=="display_fonts") NGlobal::SetVar("ui_vector_fonts",NGlobal::GetVar("ui_vector_fonts",1).GetFloat()!=0?0.0f:1.0f);
  else return false;
  UpdateDisplayPanels(); return true;
}
}
