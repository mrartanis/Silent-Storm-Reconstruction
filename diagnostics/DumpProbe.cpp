#include <windows.h>
__declspec(noinline) void DiagnosticCrashLeaf() { *(volatile int*)0 = 42; }
__declspec(noinline) void DiagnosticCrashCaller() { DiagnosticCrashLeaf(); }
int main(int argc, char**) { if(argc>1) { Sleep(INFINITE); return 0; } DiagnosticCrashCaller(); return 0; }
