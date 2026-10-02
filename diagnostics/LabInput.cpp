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
 const char labRoot[]="G:\\SS\\lab\\";
 const char steamOracleRoot[]="D:\\SS-lab\\steam-ui-oracle-01\\";
 if(_strnicmp(path,labRoot,sizeof(labRoot)-1)!=0 &&
    _strnicmp(path,steamOracleRoot,sizeof(steamOracleRoot)-1)!=0){
   fprintf(stderr,"Target is outside lab: %s\n",path);return 3;
 }
 EnumWindows(Find,(LPARAM)pid);if(!target){fprintf(stderr,"No visible target window\n");return 3;}
 if(GetForegroundWindow()!=target){
   ShowWindow(target,SW_RESTORE);
   // A real Alt press gives the lab helper foreground permission on Windows.
   INPUT alt={};alt.type=INPUT_KEYBOARD;alt.ki.wVk=VK_MENU;
   if(!Send(alt))return 4;alt.ki.dwFlags=KEYEVENTF_KEYUP;if(!Send(alt))return 4;
   SetForegroundWindow(target);Sleep(250);
 }
 if(GetForegroundWindow()!=target){fprintf(stderr,"Target not foreground; no input sent\n");return 4;}
 INPUT i={};bool ok=true;
 if(!strcmp(argv[2],"focus"))return 0;
 if(!strcmp(argv[2],"move")&&argc==5){i.type=INPUT_MOUSE;i.mi.dx=atoi(argv[3]);i.mi.dy=atoi(argv[4]);if(abs(i.mi.dx)>200||abs(i.mi.dy)>200)return 2;i.mi.dwFlags=MOUSEEVENTF_MOVE;ok=Send(i);}
 else if(!strcmp(argv[2],"key")&&(argc==4||argc==5)){unsigned sc=strtoul(argv[3],0,0);unsigned hold=argc==5?strtoul(argv[4],0,10):80;if((sc&255)==0||(sc&255)>0x58||sc>0x158||hold>2000)return 2;i.type=INPUT_KEYBOARD;i.ki.wScan=(WORD)(sc&255);i.ki.dwFlags=KEYEVENTF_SCANCODE|((sc&256)?KEYEVENTF_EXTENDEDKEY:0);ok=Send(i);Sleep(hold);i.ki.dwFlags|=KEYEVENTF_KEYUP;ok=Send(i)&&ok;}
 else if((!strcmp(argv[2],"rdrag")||!strcmp(argv[2],"ldrag"))&&argc==5){int dx=atoi(argv[3]),dy=atoi(argv[4]);if(abs(dx)>200||abs(dy)>200)return 2;i.type=INPUT_MOUSE;const bool left=!strcmp(argv[2],"ldrag");i.mi.dwFlags=left?MOUSEEVENTF_LEFTDOWN:MOUSEEVENTF_RIGHTDOWN;ok=Send(i);Sleep(80);i.mi.dx=dx;i.mi.dy=dy;i.mi.dwFlags=MOUSEEVENTF_MOVE;ok=Send(i)&&ok;Sleep(80);i.mi.dx=i.mi.dy=0;i.mi.dwFlags=left?MOUSEEVENTF_LEFTUP:MOUSEEVENTF_RIGHTUP;ok=Send(i)&&ok;}
 else if(!strcmp(argv[2],"ldown")||!strcmp(argv[2],"lup")){i.type=INPUT_MOUSE;i.mi.dwFlags=!strcmp(argv[2],"ldown")?MOUSEEVENTF_LEFTDOWN:MOUSEEVENTF_LEFTUP;ok=Send(i);Sleep(80);}
 else if(!strcmp(argv[2],"click")||!strcmp(argv[2],"doubleclick")){i.type=INPUT_MOUSE;const int count=!strcmp(argv[2],"doubleclick")?2:1;for(int n=0;n<count;++n){i.mi.dwFlags=MOUSEEVENTF_LEFTDOWN;ok=Send(i)&&ok;Sleep(80);i.mi.dwFlags=MOUSEEVENTF_LEFTUP;ok=Send(i)&&ok;if(n+1<count)Sleep(80);}}
 else return 2;
 printf("PID=%lu action=%s success=%d\n",pid,argv[2],ok);return ok?0:5;
}
