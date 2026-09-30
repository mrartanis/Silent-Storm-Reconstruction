#include "../Input/StdAfx.h"
#include "../Input/Input.h"
#include "../Input/Bind.h"
#include "../Input/SDLKeyTable.h"
#include "../Game/Platform.h"
#include "../Misc/HPTimer.h"
#include "../MiscDll/Commands.h"
#include <SDL3/SDL.h>
#include <cstdio>
#include <cmath>
#include <fstream>

namespace {
int failures = 0;
void Check(bool ok, const char* name)
{
  if (!ok) { std::fprintf(stderr, "FAIL: %s\n", name); ++failures; }
}
void Send(SDL_Event event)
{
  Check(SDL_PushEvent(&event), "SDL_PushEvent");
}
void WindowEvent(Uint32 type)
{
  SDL_Event event{}; event.type = type;
  event.window.windowID = SDL_GetWindowID(S2Platform::Window()); Send(event);
}
void Key(SDL_Scancode scan, bool down, bool repeat = false)
{
  SDL_Event event{}; event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
  event.key.windowID = SDL_GetWindowID(S2Platform::Window());
  event.key.scancode = scan; event.key.repeat = repeat; event.key.down = down;
  Send(event);
}
void Pump()
{
  S2Platform::Delay(2); S2Platform::PumpEvents();
  NInput::PumpMessages(S2Platform::Active());
}
std::vector<NInput::SMessage> Drain()
{
  std::vector<NInput::SMessage> result;
  NInput::SMessage message{};
  while (NInput::GetMessage(&message)) result.push_back(message);
  return result;
}
int Count(const std::vector<NInput::SMessage>& messages, NInput::EControlType type,
          int action, bool down = true)
{
  int result = 0;
  for (const auto& message : messages)
    if (message.cType == type && message.nAction == action && message.bState == down) ++result;
  return result;
}
}

int main(int argc, char** argv)
{
  S2Platform::SetErrorDialogs(false);
  if (argc == 2 && std::string(argv[1]) == "--invalid-driver") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "s2-invalid-driver");
    if (S2Platform::Init("invalid driver")) return 1;
    Check(!S2Platform::Window(), "failed init leaves no window");
    S2Platform::Done(); return failures ? 1 : 0;
  }
  if (!S2Platform::Init("Silent Storm SDL input tests", 640, 480, true)) return 1;
  Check(S2Platform::NativeWindow() != nullptr, "SDL Windows native adapter");
  Check(NInput::InitInput(true), "initialize SDL input");
  S2Platform::PumpEvents(); Drain();
  WindowEvent(SDL_EVENT_WINDOW_FOCUS_GAINED); Pump(); Drain();
  if (argc == 3 && std::string(argv[1]) == "--bindings") {
    NGlobal::LoadConfig(argv[2]);
    std::list<NInput::SBind> binds;
    NInput::GetBind("exitgame", &binds);
    Check(binds.size() == 4, "original input.cfg registers exit chords");
    NInput::GetBind("cursor_x", &binds);
    Check(!binds.empty(), "original input.cfg registers mouse axes");
    std::ifstream cfg(argv[2]); std::string line; int keysChecked = 0;
    Check(cfg.good(), "input.cfg readable");
    while (std::getline(cfg, line)) {
      if (line.compare(0, 2, "//") == 0) continue;
      std::size_t start = 0;
      while ((start = line.find('\'', start)) != std::string::npos) {
        const auto end = line.find('\'', ++start);
        if (end == std::string::npos) break;
        Check(NInput::GetControlID(line.substr(start, end - start)) != -1, "original config control exists");
        ++keysChecked; start = end + 1;
      }
    }
    Check(keysChecked > 50, "original control corpus checked");
    std::printf("original input.cfg: %d key references\n", keysChecked);
    NInput::DoneInput(); S2Platform::Done(); return failures ? 1 : 0;
  }

  for (const auto& key : NInput::sdlKeys)
    Check(NInput::GetControlID(key.name) == ((1 << 24) | key.offset), "legacy key ID");
  Check(NInput::GetControlID("MOUSE_AXIS_X") == 0 &&
        NInput::GetControlID("MOUSE_AXIS_Y") == 4 &&
        NInput::GetControlID("MOUSE_AXIS_Z") == 8 &&
        NInput::GetControlID("MOUSE_BUTTON1") == 13, "legacy mouse IDs");
  Check(NInput::GetControlID("invalid") == -1, "unknown key");

  const int actionA = NInput::GetControlID("A");
  Key(SDL_SCANCODE_A, true); Key(SDL_SCANCODE_A, true, true); Key(SDL_SCANCODE_A, false);
  Pump(); auto messages = Drain();
  Check(Count(messages, NInput::CT_KEY, actionA) == 1 &&
        Count(messages, NInput::CT_KEY, actionA, false) == 1, "one physical down/up despite repeat");
  Check(Count(messages, NInput::CT_WIN_KEY, -1) == 2, "key repeat reaches UI exactly twice");

  // SDL's text pointer expires at the next poll: the platform must own the copy.
  SDL_Event text{}; text.type = SDL_EVENT_TEXT_INPUT;
  text.text.windowID = SDL_GetWindowID(S2Platform::Window());
  text.text.text = "a\xD0\x96\xF0\x9F\x98\x80";
  Send(text); Pump(); messages = Drain();
  Check(messages.size() == 4 && messages[0].nParam == 'a' && messages[1].nParam == 0x416 &&
        messages[2].nParam == 0xd83d && messages[3].nParam == 0xde00, "Unicode text and UTF-16 surrogate pair");
  for (const auto& message : messages) Check(message.cType == NInput::CT_WIN_CHAR, "text is not a physical key");

  SDL_Event motion{}; motion.type = SDL_EVENT_MOUSE_MOTION;
  motion.motion.windowID = SDL_GetWindowID(S2Platform::Window());
  motion.motion.xrel = 12; motion.motion.yrel = -4; Send(motion);
  SDL_Event wheel{}; wheel.type = SDL_EVENT_MOUSE_WHEEL;
  wheel.wheel.windowID = motion.motion.windowID; wheel.wheel.y = 1; Send(wheel);
  Pump(); messages = Drain();
  Check(messages.size() == 3 && messages[0].nAction == 0 && messages[0].nParam == 12 &&
        messages[1].nAction == 4 && messages[1].nParam == -4 &&
        messages[2].nAction == 8 && messages[2].nParam == 120, "relative axes and legacy wheel scale");

  // Exercise real CBind chord state and its release path, not a mock translator.
  NInput::CBind quit("sdl_test_quit");
  NGlobal::ProcessCommand(L"bind sdl_test_quit 'LCTRL' + 'Q'");
  NInput::UpdateBinds();
  Key(SDL_SCANCODE_LCTRL, true); Key(SDL_SCANCODE_Q, true); Pump();
  NInput::SEvent event{}; int commands = 0;
  while (NInput::GetEvent(&event)) if (quit.ProcessEvent(event)) ++commands;
  Check(commands == 1, "configured chord executes once");
  NInput::CBind forward("sdl_test_forward");
  NGlobal::ProcessCommand(L"bind +sdl_test_forward 'UP'");
  NInput::UpdateBinds();
  SDL_Event button{}; button.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
  button.button.windowID = SDL_GetWindowID(S2Platform::Window());
  button.button.button = SDL_BUTTON_RIGHT; button.button.down = true; Send(button);
  Key(SDL_SCANCODE_UP, true); Pump();
  while (NInput::GetEvent(&event)) forward.ProcessEvent(event);
  // The first sample shares the key-down tick; measure a subsequent held frame.
  Pump();
  while (NInput::GetEvent(&event)) forward.ProcessEvent(event);
  Check(forward.GetSpeed() > 0, "held keyboard slider is active");
  WindowEvent(SDL_EVENT_WINDOW_FOCUS_LOST); Pump();
  messages.clear();
  while (NInput::GetEvent(&event)) {
    messages.push_back(event.mMessage);
    quit.ProcessEvent(event); forward.ProcessEvent(event);
  }
  Check(Count(messages, NInput::CT_KEY, NInput::GetControlID("LCTRL"), false) == 1 &&
        Count(messages, NInput::CT_KEY, NInput::GetControlID("Q"), false) == 1, "focus loss releases every held key");
  Check(Count(messages, NInput::CT_KEY, 13, false) == 1, "focus loss releases held mouse button");
  Check(!SDL_GetWindowMouseGrab(S2Platform::Window()), "focus loss releases mouse grab");
  Check(forward.GetSpeed() == 0, "focus loss resets configured keyboard slider");
  Key(SDL_SCANCODE_A, true); Pump(); Check(Drain().empty(), "background input is ignored");

  WindowEvent(SDL_EVENT_WINDOW_FOCUS_GAINED);
  Key(SDL_SCANCODE_Q, true); Pump(); commands = 0;
  while (NInput::GetEvent(&event)) if (quit.ProcessEvent(event)) ++commands;
  Check(commands == 0, "released modifier cannot complete a chord after focus restoration");
  Key(SDL_SCANCODE_Q, false); Key(SDL_SCANCODE_LCTRL, true);
  Key(SDL_SCANCODE_Q, true); Pump(); commands = 0;
  while (NInput::GetEvent(&event)) if (quit.ProcessEvent(event)) ++commands;
  Check(commands == 1, "configured chord works again after focus restoration");
  Key(SDL_SCANCODE_Q, false); Key(SDL_SCANCODE_LCTRL, false); Pump();
  while (NInput::GetEvent(&event)) quit.ProcessEvent(event);

  WindowEvent(SDL_EVENT_WINDOW_FOCUS_GAINED); Key(SDL_SCANCODE_A, true); Pump();
  Check(Count(Drain(), NInput::CT_KEY, actionA) == 1, "focus restoration permits fresh input");
  WindowEvent(SDL_EVENT_WINDOW_FOCUS_LOST); WindowEvent(SDL_EVENT_WINDOW_FOCUS_GAINED);
  Key(SDL_SCANCODE_A, true); Pump(); messages = Drain();
  Check(Count(messages, NInput::CT_KEY, actionA, false) == 1 &&
        Count(messages, NInput::CT_KEY, actionA) == 1, "loss and restoration in one pump preserve order");

  NHPTimer::STime before; NHPTimer::GetTime(&before);
  const auto ticks = S2Platform::Milliseconds(); S2Platform::Delay(20);
  const double seconds = NHPTimer::GetTimePassed(&before);
  Check(seconds >= .010 && seconds < 1 && S2Platform::Milliseconds() - ticks >= 10,
        "monotonic clocks retain seconds and milliseconds");
  Check(std::fabs(NHPTimer::GetSeconds(1000000000) - 1) < 1e-12, "high-resolution clock units");

  Check(S2Platform::SetMode(800, 600, false), "resize SDL window");
  int width = 0, height = 0; S2Platform::Size(&width, &height);
  Check(width == 800 && height == 600, "drawable dimensions after resize");
  WindowEvent(SDL_EVENT_WINDOW_MINIMIZED); Pump();
  Check(!S2Platform::Active(), "minimized window pauses input");
  WindowEvent(SDL_EVENT_WINDOW_CLOSE_REQUESTED); Pump();
  Check(S2Platform::Exiting(), "window close requests exit");
  NInput::DoneInput(); S2Platform::Done(); S2Platform::Done();
  Check(!S2Platform::Window(), "idempotent cleanup");
  Check(S2Platform::Init("Silent Storm SDL restart", 640, 480, true), "restart after cleanup");
  Check(!S2Platform::Exiting(), "restart resets exit flag");
  S2Platform::Done();
  std::printf("SDL platform/input: %d failures\n", failures);
  return failures ? 1 : 0;
}
