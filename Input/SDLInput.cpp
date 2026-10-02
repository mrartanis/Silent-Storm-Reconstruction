#include "StdAfx.h"
#include "Input.h"
#include "SDLKeyTable.h"
#include "../Game/Platform.h"
#include <SDL3/SDL.h>
#include <array>

namespace NInput {
namespace {
bool initialized = false, focused = false, exclusive = true;
std::uint32_t lastPump = 0;
std::list<SMessage> messages;
std::array<bool, 256> keys{};
std::array<bool, 8> buttons{};
float motionX = 0, motionY = 0, wheelRemainder = 0;
int KeyboardAction(int offset) { return (1 << 24) | offset; }

int VirtualKey(SDL_Scancode scan)
{
  // Existing UI events use historical virtual-key numbers, not SDL keycodes.
  if (scan >= SDL_SCANCODE_A && scan <= SDL_SCANCODE_Z) return 'A' + scan - SDL_SCANCODE_A;
  if (scan >= SDL_SCANCODE_1 && scan <= SDL_SCANCODE_9) return '1' + scan - SDL_SCANCODE_1;
  if (scan >= SDL_SCANCODE_F1 && scan <= SDL_SCANCODE_F12) return 0x70 + scan - SDL_SCANCODE_F1;
  if (scan >= SDL_SCANCODE_F13 && scan <= SDL_SCANCODE_F15) return 0x7c + scan - SDL_SCANCODE_F13;
  if (scan >= SDL_SCANCODE_KP_1 && scan <= SDL_SCANCODE_KP_9) return 0x61 + scan - SDL_SCANCODE_KP_1;
  switch (scan) {
    case SDL_SCANCODE_0: return '0';
    case SDL_SCANCODE_BACKSPACE: return 8;
    case SDL_SCANCODE_TAB: return 9;
    case SDL_SCANCODE_RETURN: case SDL_SCANCODE_KP_ENTER: return 13;
    case SDL_SCANCODE_LSHIFT: case SDL_SCANCODE_RSHIFT: return 16;
    case SDL_SCANCODE_LCTRL: case SDL_SCANCODE_RCTRL: return 17;
    case SDL_SCANCODE_LALT: case SDL_SCANCODE_RALT: return 18;
    case SDL_SCANCODE_PAUSE: return 19;
    case SDL_SCANCODE_CAPSLOCK: return 20;
    case SDL_SCANCODE_ESCAPE: return 27;
    case SDL_SCANCODE_SPACE: return 32;
    case SDL_SCANCODE_PAGEUP: return 33;
    case SDL_SCANCODE_PAGEDOWN: return 34;
    case SDL_SCANCODE_END: return 35;
    case SDL_SCANCODE_HOME: return 36;
    case SDL_SCANCODE_LEFT: return 37;
    case SDL_SCANCODE_UP: return 38;
    case SDL_SCANCODE_RIGHT: return 39;
    case SDL_SCANCODE_DOWN: return 40;
    case SDL_SCANCODE_PRINTSCREEN: return 44;
    case SDL_SCANCODE_INSERT: return 45;
    case SDL_SCANCODE_DELETE: return 46;
    case SDL_SCANCODE_LGUI: return 91;
    case SDL_SCANCODE_RGUI: return 92;
    case SDL_SCANCODE_APPLICATION: return 93;
    case SDL_SCANCODE_KP_0: return 96;
    case SDL_SCANCODE_KP_MULTIPLY: return 106;
    case SDL_SCANCODE_KP_PLUS: return 107;
    case SDL_SCANCODE_KP_MINUS: return 109;
    case SDL_SCANCODE_KP_PERIOD: return 110;
    case SDL_SCANCODE_KP_DIVIDE: return 111;
    case SDL_SCANCODE_NUMLOCKCLEAR: return 144;
    case SDL_SCANCODE_SCROLLLOCK: return 145;
    case SDL_SCANCODE_SEMICOLON: return 186;
    case SDL_SCANCODE_EQUALS: return 187;
    case SDL_SCANCODE_COMMA: return 188;
    case SDL_SCANCODE_MINUS: return 189;
    case SDL_SCANCODE_PERIOD: return 190;
    case SDL_SCANCODE_SLASH: return 191;
    case SDL_SCANCODE_GRAVE: return 192;
    case SDL_SCANCODE_LEFTBRACKET: return 219;
    case SDL_SCANCODE_BACKSLASH: return 220;
    case SDL_SCANCODE_RIGHTBRACKET: return 221;
    case SDL_SCANCODE_APOSTROPHE: return 222;
    case SDL_SCANCODE_NONUSBACKSLASH: return 226;
    default: return 0;
  }
}
void Push(EControlType type, int action, int value, bool down = true)
{
  SMessage message{};
  message.cType = type; message.nAction = action; message.nParam = value;
  message.bState = down; message.ePOVAxis = PA_UNKNOWN;
  message.tTime = S2Platform::Milliseconds();
  messages.push_back(message);
}
void ReleaseAll()
{
  for (int i = 0; i < 256; ++i) if (keys[i]) {
    keys[i] = false; Push(CT_KEY, KeyboardAction(i), -128, false);
  }
  for (int i = 0; i < 8; ++i) if (buttons[i]) {
    buttons[i] = false; Push(CT_KEY, 12 + i, -128, false);
  }
  motionX = motionY = wheelRemainder = 0;
  S2Platform::CaptureMouse(false);
}
int MouseButton(int button)
{
  if (button == SDL_BUTTON_LEFT) return 0;
  if (button == SDL_BUTTON_RIGHT) return 1;
  if (button == SDL_BUTTON_MIDDLE) return 2;
  return button >= 4 && button <= 8 ? button - 1 : -1;
}
void Axis(float* remainder, int action, float delta)
{
  *remainder += delta;
  const int whole = static_cast<int>(*remainder);
  *remainder -= whole;
  if (whole) Push(CT_AXIS, action, whole);
}
}

bool InitInput(bool nonExclusive, int)
{
  if (initialized) return true;
  if (!S2Platform::Window()) return false;
  messages.clear(); keys.fill(false); buttons.fill(false);
  motionX = motionY = wheelRemainder = 0;
  exclusive = !nonExclusive; focused = S2Platform::Active();
  lastPump = S2Platform::Milliseconds() - 1;
  initialized = true;
  S2Platform::CaptureMouse(focused && exclusive);
  return true;
}
bool DoneInput()
{
  if (initialized) ReleaseAll();
  initialized = focused = false;
  messages.clear(); return true;
}
void PumpMessages(bool focus)
{
  if (!initialized) return;
  // Keep motion on clock-advanced frames: the camera discards dt==0 deltas.
  const std::uint32_t now = S2Platform::Milliseconds();
  if (!focus && focused) { ReleaseAll(); focused = false; }
  if (focus && !focused) { focused = true; S2Platform::CaptureMouse(exclusive); }
  if (focus && now - lastPump < 1) return;
  lastPump = now;
  S2Platform::InputEvent input;
  while (S2Platform::PollInput(&input)) {
    const SDL_Event& event = input.event;
    if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST ||
        event.type == SDL_EVENT_WINDOW_MINIMIZED || event.type == SDL_EVENT_WINDOW_HIDDEN) {
      ReleaseAll(); focused = false; continue;
    }
    if (event.type == SDL_EVENT_WINDOW_FOCUS_GAINED) {
      focused = true; S2Platform::CaptureMouse(exclusive); continue;
    }
    if (!focused) continue;
    switch (event.type) {
      case SDL_EVENT_KEY_DOWN: case SDL_EVENT_KEY_UP: {
        const bool down = event.type == SDL_EVENT_KEY_DOWN;
        for (const SDLKeyInfo& key : sdlKeys) {
          if (key.scancode == SDL_SCANCODE_UNKNOWN || key.scancode != event.key.scancode) continue;
          if (!event.key.repeat && keys[key.offset] != down) {
            keys[key.offset] = down;
            Push(CT_KEY, KeyboardAction(key.offset), down ? 128 : -128, down);
          }
        }
        const int vk = VirtualKey(event.key.scancode);
        if (down && vk) Push(CT_WIN_KEY, -1, vk);
        break;
      }
      case SDL_EVENT_TEXT_INPUT: {
        const std::uint32_t c = input.character;
        // Windows UI stores UTF-16; portable UI can accept full scalars.
#if defined(_WIN32)
        if (c > 0xffff) {
          Push(CT_WIN_CHAR, -1, 0xd800 + ((c - 0x10000) >> 10));
          Push(CT_WIN_CHAR, -1, 0xdc00 + ((c - 0x10000) & 0x3ff));
        } else
#endif
          Push(CT_WIN_CHAR, -1, static_cast<int>(c));
        break;
      }
      case SDL_EVENT_MOUSE_MOTION:
        Axis(&motionX, 0, S2Platform::Display().WindowToPixelX(event.motion.xrel)); Axis(&motionY, 4, S2Platform::Display().WindowToPixelY(event.motion.yrel)); break;
      case SDL_EVENT_MOUSE_WHEEL:
        // Legacy binds expect +/-120 per wheel detent.
        Axis(&wheelRemainder, 8, event.wheel.y *
            (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -120.0f : 120.0f)); break;
      case SDL_EVENT_MOUSE_BUTTON_DOWN: case SDL_EVENT_MOUSE_BUTTON_UP: {
        const int button = MouseButton(event.button.button);
        const bool down = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
        if (button >= 0 && buttons[button] != down) {
          buttons[button] = down; Push(CT_KEY, 12 + button, down ? 128 : -128, down);
        }
        break;
      }
      default: break;
    }
  }
}
bool GetMessage(SMessage* message)
{
  if (messages.empty()) {
    *message = SMessage{}; message->cType = CT_TIME;
    message->tTime = S2Platform::Milliseconds(); return false;
  }
  *message = messages.front(); messages.pop_front(); return true;
}
void AddWinMessage(EControlType type, int parameter) { Push(type, -1, parameter); }
bool GetKeyForMessage(const SMessage& message, int* key)
{
  *key = 0;
  if (message.cType != CT_KEY || !message.bState) return false;
  for (const SDLKeyInfo& info : sdlKeys) if (KeyboardAction(info.offset) == message.nAction) {
    *key = VirtualKey(info.scancode); return *key != 0;
  }
  return false;
}
int GetControlID(const string& name)
{
  for (const SDLKeyInfo& key : sdlKeys) if (name == key.name) return KeyboardAction(key.offset);
  if (name == "MOUSE_AXIS_X") return 0;
  if (name == "MOUSE_AXIS_Y") return 4;
  if (name == "MOUSE_AXIS_Z") return 8;
  for (int i = 0; i < 8; ++i)
    if (name == string("MOUSE_BUTTON") + static_cast<char>('0' + i)) return 12 + i;
  return -1;
}
void GetControlInfo(int action, EControlType* type, float* granularity)
{
  *type = CT_UNKNOWN; *granularity = 1;
  if (action == 0 || action == 4 || action == 8) { *type = CT_AXIS; return; }
  if (action >= 12 && action <= 19) { *type = CT_KEY; return; }
  for (const SDLKeyInfo& key : sdlKeys)
    if (action == KeyboardAction(key.offset)) { *type = CT_KEY; return; }
}
// These hooks were empty in the reconstructed backend. Preserve that API;
// stage 3 does not invent an incompatible recording format.
void StartSaveInput(CDataStream*) {}
void StopSaveInput() {}
void StartEmulateInput(CDataStream*) {}
void StopEmulateInput() {}
}
