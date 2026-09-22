#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static HWND target;
static BOOL CALLBACK Find(HWND w, LPARAM p) { DWORD pid=0; GetWindowThreadProcessId(w,&pid); if(pid==(DWORD)p && IsWindowVisible(w) && !GetWindow(w,GW_OWNER)){target=w;return FALSE;}return TRUE; }
static bool Send(INPUT& i){UINT n=SendInput(1,&i,sizeof(i));if(n!=1){fprintf(stderr,"SendInput failed: %lu\n",GetLastError());return false;}return true;}
int main(int argc,char**argv){
 if(argc<3){fprintf(stderr,"LabInput PID focus|move dx dy|key scancode|click\n");return 2;}
 DWORD pid=strtoul(argv[1],0,10);HANDLE h=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);char path[MAX_PATH]={};DWORD size=MAX_PATH;
 if(!h||!QueryFullProcessImageNameA(h,0,path,&size)){fprintf(stderr,"Cannot identify target\n");return 3;}CloseHandle(h);
 if(_strnicmp(path,"G:\\SS\\lab\\",10)!=0){fprintf(stderr,"Target is outside lab\n");return 3;}
 EnumWindows(Find,(LPARAM)pid);if(!target){fprintf(stderr,"No visible target window\n");return 3;}
 if(GetForegroundWindow()!=target){ShowWindow(target,SW_RESTORE);SetForegroundWindow(target);Sleep(250);}
 if(GetForegroundWindow()!=target){fprintf(stderr,"Target not foreground; no input sent\n");return 4;}
 INPUT i={};bool ok=true;
 if(!strcmp(argv[2],"focus"))return 0;
 if(!strcmp(argv[2],"move")&&argc==5){i.type=INPUT_MOUSE;i.mi.dx=atoi(argv[3]);i.mi.dy=atoi(argv[4]);if(abs(i.mi.dx)>200||abs(i.mi.dy)>200)return 2;i.mi.dwFlags=MOUSEEVENTF_MOVE;ok=Send(i);}
 else if(!strcmp(argv[2],"key")&&(argc==4||argc==5)){unsigned sc=strtoul(argv[3],0,0);unsigned hold=argc==5?strtoul(argv[4],0,10):80;if((sc&255)==0||(sc&255)>0x58||sc>0x158||hold>2000)return 2;i.type=INPUT_KEYBOARD;i.ki.wScan=(WORD)(sc&255);i.ki.dwFlags=KEYEVENTF_SCANCODE|((sc&256)?KEYEVENTF_EXTENDEDKEY:0);ok=Send(i);Sleep(hold);i.ki.dwFlags|=KEYEVENTF_KEYUP;ok=Send(i)&&ok;}
 else if(!strcmp(argv[2],"rdrag")&&argc==5){int dx=atoi(argv[3]),dy=atoi(argv[4]);if(abs(dx)>200||abs(dy)>200)return 2;i.type=INPUT_MOUSE;i.mi.dwFlags=MOUSEEVENTF_RIGHTDOWN;ok=Send(i);Sleep(80);i.mi.dx=dx;i.mi.dy=dy;i.mi.dwFlags=MOUSEEVENTF_MOVE;ok=Send(i)&&ok;Sleep(80);i.mi.dx=i.mi.dy=0;i.mi.dwFlags=MOUSEEVENTF_RIGHTUP;ok=Send(i)&&ok;}
 else if(!strcmp(argv[2],"click")){i.type=INPUT_MOUSE;i.mi.dwFlags=MOUSEEVENTF_LEFTDOWN;ok=Send(i);Sleep(80);i.mi.dwFlags=MOUSEEVENTF_LEFTUP;ok=Send(i)&&ok;}
 else return 2;
 printf("PID=%lu action=%s success=%d\n",pid,argv[2],ok);return ok?0:5;
}
