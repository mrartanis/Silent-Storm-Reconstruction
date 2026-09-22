#include <windows.h>
#include <stdio.h>
static int keys=0, clicks=0;
LRESULT CALLBACK Proc(HWND w,UINT m,WPARAM a,LPARAM b){
 if(m==WM_KEYDOWN || m==WM_LBUTTONDOWN){if(m==WM_KEYDOWN) ++keys;else ++clicks; FILE*f=fopen("input-events.log","a");if(f){fprintf(f,"message=%u value=%u x=%d y=%d\n",m,(unsigned)a,LOWORD(b),HIWORD(b));fclose(f);}InvalidateRect(w,0,TRUE);}
 if(m==WM_PAINT){PAINTSTRUCT p;HDC d=BeginPaint(w,&p);char t[128];sprintf(t,"Input probe: keys=%d clicks=%d",keys,clicks);TextOutA(d,20,25,t,(int)strlen(t));EndPaint(w,&p);return 0;}
 if(m==WM_DESTROY){PostQuitMessage(0);return 0;}return DefWindowProc(w,m,a,b);
}
int WINAPI WinMain(HINSTANCE h,HINSTANCE,LPSTR,int){WNDCLASSA c={};c.lpfnWndProc=Proc;c.hInstance=h;c.lpszClassName="SSLabInputProbe";c.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);c.hCursor=LoadCursor(0,IDC_ARROW);RegisterClassA(&c);HWND w=CreateWindowA(c.lpszClassName,"SS Lab Input Probe",WS_OVERLAPPEDWINDOW,200,200,640,250,0,0,h,0);ShowWindow(w,SW_SHOW);MSG m;while(GetMessage(&m,0,0,0)>0){TranslateMessage(&m);DispatchMessage(&m);}return 0;}
