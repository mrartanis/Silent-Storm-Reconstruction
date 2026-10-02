#include "StdAfx.h"
#include "Interface.h"
#include "DisplayLayout.h"
#include "DisplayOptions.h"
#include "GInit.h"
#include "../MiscDll/Commands.h"

namespace NUI {
namespace {
CWindow* Panel(CInterface* ui, const char* id, int height) {
  CWindow* old = ui->GetChildByID(id);
  if (IsValid(old)) { old->ShowWindow(SWTYPE_SHOW); return old; }
  auto* panel = new CWindow(SWindowInfo(ui, SPoint(240, (768-height)/2), SPoint(544,height), id,
      STYLE_ENABLED | STYLE_VISIBLE | STYLE_TOPMOST | STYLE_MODAL));
  panel->EnableAdaptiveLayout();
  auto* background = new CImage(SWindowInfo(panel, SPoint(0,0), SPoint(544,height), "background",
      STYLE_ENABLED | STYLE_VISIBLE | STYLE_TRANSPARENT | STYLE_BOTTOMMOST));
  background->SetColor(NGfx::SPixel8888(12,16,20,245));
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
    const bool fullscreen=NGlobal::GetVar("gfx_fullscreen",1).GetFloat()!=0;
    Button(settings,"display_mode",58,fullscreen?L"<font size=18pt>Window mode: Borderless fullscreen":L"<font size=18pt>Window mode: Window");
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
    WCHAR text[256]; swprintf(text,256,L"<font size=20pt>Keep this display mode? Reverting in %d seconds.",seconds);
    Label(confirmation,"countdown",20,text);
    Button(confirmation,"display_keep",92,L"<font size=18pt><center>Keep (Enter)");
    Button(confirmation,"display_revert",140,L"<font size=18pt><center>Revert (Escape)");
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
    NGlobal::SetVar("gfx_fullscreen",NGlobal::GetVar("gfx_fullscreen",1).GetFloat()!=0?0.0f:1.0f);
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
