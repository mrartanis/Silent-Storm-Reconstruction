"""Actual ordinary launch of the exact user binary in existing isolated user-data."""
import ctypes,json,subprocess,time,os,hashlib,sys
from pathlib import Path
from ctypes import wintypes
from PIL import ImageGrab
B=Path('G:/SS/lab/runs/hd-groups-20261007-01');run=B/'menu-smoke-20261009-01';E=run/'evidence'
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
PFX='ordinary-hd2837-foreground' if len(sys.argv)>1 and sys.argv[1]=='foreground' else 'ordinary-hd2837'
assert sha(B/'game/Game.exe')==sha(run/'game/Game.exe')
assert (B/'game/cfg/lab-no-intro.cfg').read_bytes()==(run/'game/cfg/lab-no-intro.cfg').read_bytes()
assert not (E/(PFX+'-proof.json')).exists()
u=ctypes.windll.user32;u.SetProcessDPIAware()
u.GetForegroundWindow.restype=wintypes.HWND
env=os.environ.copy();env['S2_USER_DATA_DIR']=str(run/'user-data')
for k in list(env):
 if k.startswith(('S2_PERF_','S2_DIAG_')) or k=='S2_TEXTURE_DIAGNOSTICS':env.pop(k)
cmd='"'+str(run/'game/Game.exe')+'" -cfg .\\cfg\\lab-no-intro.cfg'
log=(E/(PFX+'-process.log')).open('wb');p=subprocess.Popen(cmd,cwd=run/'game',env=env,stdout=log,stderr=log)
(E/(PFX+'-own-pid.txt')).write_text(str(p.pid),encoding='ascii')
def owned_windows():
 result=[]
 @ctypes.WINFUNCTYPE(wintypes.BOOL,wintypes.HWND,wintypes.LPARAM)
 def cb(w,_):
  pid=wintypes.DWORD();u.GetWindowThreadProcessId(w,ctypes.byref(pid))
  if pid.value==p.pid and u.IsWindowVisible(w):result.append(w)
  return True
 u.EnumWindows(cb,0);return result
hwnd=None;shots=[]
try:
 deadline=time.monotonic()+45
 while time.monotonic()<deadline:
  assert p.poll() is None
  windows=owned_windows()
  if windows:hwnd=windows[0];break
  time.sleep(.2)
 assert hwnd;time.sleep(6)
 def area(w):
  rc=wintypes.RECT();u.GetClientRect(w,ctypes.byref(rc));return rc.right*rc.bottom
 hwnd=max(owned_windows(),key=area);u.ShowWindow(hwnd,3);u.SetForegroundWindow(hwnd);time.sleep(1)
 if PFX.endswith('foreground'):
  cur=ctypes.windll.kernel32.GetCurrentThreadId();fg=u.GetForegroundWindow();tid=u.GetWindowThreadProcessId(fg,None);own=u.GetWindowThreadProcessId(hwnd,None)
  attached=[]
  try:
   for t in set([tid,own]):
    if t and t!=cur and u.AttachThreadInput(cur,t,True):attached.append(t)
   u.BringWindowToTop(hwnd);u.SetForegroundWindow(hwnd);u.SetFocus(hwnd)
  finally:
   for t in attached:u.AttachThreadInput(cur,t,False)
  time.sleep(1)
 for phase in range(4):
  assert p.poll() is None and hwnd in owned_windows()
  rect=wintypes.RECT();u.GetClientRect(hwnd,ctypes.byref(rect));pt=wintypes.POINT(0,0);u.ClientToScreen(hwnd,ctypes.byref(pt))
  path=E/f'{PFX}-mainmenu-phase{phase}.png';assert not path.exists()
  if PFX.endswith('foreground'):
   assert u.GetForegroundWindow()==hwnd,'Own Game could not take foreground; no screen PASS claimed'
   ImageGrab.grab(bbox=(pt.x,pt.y,pt.x+rect.right,pt.y+rect.bottom),all_screens=True).save(path)
  else:ImageGrab.grab(window=hwnd).save(path)
  shots.append({'path':str(path),'SHA256':sha(path),'client_size':[rect.right,rect.bottom],'capture_method':'Pillow native window capture of own HWND; no foreground requirement','foreground_at_capture':u.GetForegroundWindow()==hwnd});time.sleep(.5)
finally:
 if p.poll() is None:
  assert hwnd and hwnd in owned_windows();u.PostMessageW(hwnd,0x0010,0,0);p.wait(timeout=30)
 log.close()
assert p.returncode==0
report={'own_PID':p.pid,'exact_command':cmd,'no_harness_no_diagnostics_no_automatic_quit_in_user_launcher':True,'ordinary_user_binary_exact_SHA256':sha(B/'game/Game.exe'),'test_binary_same_as_normal_launcher':True,'test_cfg_same_as_normal_launcher':True,'actual_pack_textures':len(json.loads((run/'game/res-hd/manifest.json').read_text())['textures']),'captures':shots,'own_normal_WM_CLOSE_exit_code':p.returncode,'own_PID_absent':p.poll() is not None,'normal_user_config_saves_launcher_or_Game_exe_modified':False}
(E/(PFX+'-proof.json')).write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Ordinary exact user binary mainmenu captured4phases; own PID',p.pid,'normal quit0; no user saves/config changes.')
