#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <dinput.h>
#include <stdio.h>
static IDirectInput8A* di; static IDirectInputDevice8A* kb; static IDirectInputDevice8A* ms;
static int keys=0,clicks=0,diKeys=0,diClicks=0;static LONG dx=0,dy=0;
static void Log(const char* kind,DWORD offset,DWORD data){FILE*f=fopen("directinput-events.log","a");if(f){fprintf(f,"%s offset=%lu data=%lu\n",kind,offset,data);fclose(f);}}
static void Poll(IDirectInputDevice8A*d,bool mouse){DIDEVICEOBJECTDATA ev[64];DWORD count=64;HRESULT r=d->GetDeviceData(sizeof(ev[0]),ev,&count,0);static HRESULT lastKeyboard=E_FAIL,lastMouse=E_FAIL; HRESULT &last=mouse?lastMouse:lastKeyboard; if(r!=last){Log(mouse?"mouse HRESULT":"keyboard HRESULT",0,(DWORD)r);last=r;} if(FAILED(r)){d->Acquire();return;}for(DWORD i=0;i<count;i++){Log(mouse?"mouse":"keyboard",ev[i].dwOfs,ev[i].dwData);if(mouse){if(ev[i].dwOfs==DIMOFS_X)dx+=(LONG)ev[i].dwData;else if(ev[i].dwOfs==DIMOFS_Y)dy+=(LONG)ev[i].dwData;else ++diClicks;}else ++diKeys;}}
LRESULT CALLBACK Proc(HWND w,UINT m,WPARAM a,LPARAM b){
 if(m==WM_KEYDOWN||m==WM_LBUTTONDOWN){if(m==WM_KEYDOWN)++keys;else ++clicks;Log("window",m,(DWORD)b);InvalidateRect(w,0,TRUE);}
 if(m==WM_TIMER){Poll(kb,false);Poll(ms,true);InvalidateRect(w,0,TRUE);}
 if(m==WM_PAINT){PAINTSTRUCT p;HDC d=BeginPaint(w,&p);char t[200];sprintf(t,"Win32 keys=%d clicks=%d; DirectInput keys=%d buttons=%d dx=%ld dy=%ld",keys,clicks,diKeys,diClicks,dx,dy);TextOutA(d,10,25,t,(int)strlen(t));EndPaint(w,&p);return 0;}
 if(m==WM_DESTROY){PostQuitMessage(0);return 0;}return DefWindowProc(w,m,a,b);
}
int WINAPI WinMain(HINSTANCE h,HINSTANCE,LPSTR args,int){WNDCLASSA c={};c.lpfnWndProc=Proc;c.hInstance=h;c.lpszClassName="SSLabDIProbe";c.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);c.hCursor=LoadCursor(0,IDC_ARROW);RegisterClassA(&c);HWND w=CreateWindowA(c.lpszClassName,"SS Lab DirectInput Probe",WS_OVERLAPPEDWINDOW,100,200,900,250,0,0,h,0);DirectInput8Create(h,DIRECTINPUT_VERSION,IID_IDirectInput8A,(void**)&di,0);di->CreateDevice(GUID_SysKeyboard,&kb,0);di->CreateDevice(GUID_SysMouse,&ms,0);kb->SetDataFormat(&c_dfDIKeyboard);ms->SetDataFormat(&c_dfDIMouse2);kb->SetCooperativeLevel(w,DISCL_NONEXCLUSIVE|DISCL_FOREGROUND);ms->SetCooperativeLevel(w,(strstr(args,"exclusive")?DISCL_EXCLUSIVE:DISCL_NONEXCLUSIVE)|DISCL_FOREGROUND);DIPROPDWORD p={};p.diph.dwSize=sizeof(p);p.diph.dwHeaderSize=sizeof(p.diph);p.diph.dwHow=DIPH_DEVICE;p.dwData=64;kb->SetProperty(DIPROP_BUFFERSIZE,&p.diph);ms->SetProperty(DIPROP_BUFFERSIZE,&p.diph);ShowWindow(w,SW_SHOW);kb->Acquire();ms->Acquire();SetTimer(w,1,16,0);MSG m;while(GetMessage(&m,0,0,0)>0){TranslateMessage(&m);DispatchMessage(&m);}kb->Unacquire();ms->Unacquire();kb->Release();ms->Release();di->Release();return 0;}


