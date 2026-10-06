#pragma once
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#ifdef _WIN32
#include <windows.h>
#else
#include <csignal>
#include <fcntl.h>
#include <unistd.h>
#endif
namespace S2Net {
static std::filesystem::path NetworkLogRoot() {
#ifdef _WIN32
  auto env=[](const wchar_t* name){wchar_t value[32768]={};GetEnvironmentVariableW(name,value,32768);return std::wstring(value);};
  auto root=env(L"S2_USER_DATA_DIR");if(root.empty()){root=env(L"LOCALAPPDATA");if(!root.empty())root+=L"\\Silent Storm Reconstruction";}
#else
  auto env=[](const char* name){auto* value=std::getenv(name);return value?std::string(value):std::string();};
  auto root=env("S2_USER_DATA_DIR");if(root.empty()){root=env("XDG_DATA_HOME");if(root.empty()){root=env("HOME");if(!root.empty())root+="/.local/share";}if(!root.empty())root+="/silent-storm-reconstruction";}
#endif
  return std::filesystem::path(root);
}
static void DiagnosticLog(const char* name,const std::string& message) noexcept {
  try {
    static std::mutex mutex;std::lock_guard<std::mutex> lock(mutex);
    auto root=NetworkLogRoot();if(root.empty() || !root.is_absolute())return;
    std::filesystem::create_directories(root);auto file=root/name;
    if(std::filesystem::exists(file) && std::filesystem::file_size(file)>1024*1024) {
      auto previous=root/(std::string(name)=="network.log"?"network.previous.log":"startup.previous.log");std::error_code error;std::filesystem::remove(previous,error);std::filesystem::rename(file,previous,error);
    }
    std::ofstream output(file,std::ios::app);
    auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    output<<ms<<" "<<message<<"\n";
  }catch(...){}
}
void NetworkLog(const std::string& message) noexcept { DiagnosticLog("network.log",message); }
void StartupLog(const std::string& message) noexcept {
  static const auto start=std::chrono::steady_clock::now();
  try {
    auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();
    DiagnosticLog("startup.log","elapsed_ms="+std::to_string(elapsed)+" "+message);
  }catch(...){}
}
#ifdef _WIN32
static HANDLE networkCrashFile=INVALID_HANDLE_VALUE;
LONG WINAPI NetworkCrashFilter(EXCEPTION_POINTERS* exception) {
  if(networkCrashFile!=INVALID_HANDLE_VALUE) {
    char message[160];int length=wsprintfA(message,"Unhandled exception code=0x%08lx address=%p\r\n",exception->ExceptionRecord->ExceptionCode,exception->ExceptionRecord->ExceptionAddress);
    DWORD written=0;WriteFile(networkCrashFile,message,length,&written,nullptr);FlushFileBuffers(networkCrashFile);
  }
  return EXCEPTION_CONTINUE_SEARCH;
}
#else
static int networkCrashFile=-1;
void NetworkCrashSignal(int signal) {
  if(networkCrashFile>=0){const char message[]="Fatal native signal; inspect system core dump\n";auto ignored=write(networkCrashFile,message,sizeof(message)-1);(void)ignored;}
  std::signal(signal,SIG_DFL);raise(signal);
}
#endif
void InstallNetworkCrashDiagnostics() noexcept {
  try {
    auto root=NetworkLogRoot();if(root.empty() || !root.is_absolute())return;std::filesystem::create_directories(root);
#ifdef _WIN32
    networkCrashFile=CreateFileW((root/L"network-crash.log").c_str(),FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);SetUnhandledExceptionFilter(NetworkCrashFilter);
#else
    networkCrashFile=open((root/"network-crash.log").c_str(),O_WRONLY|O_CREAT|O_APPEND,0600);for(int signal:{SIGSEGV,SIGABRT,SIGBUS})std::signal(signal,NetworkCrashSignal);
#endif
  }catch(...){}
}
}
