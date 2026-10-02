"""Capture real game frames while resizing the native window. Run outside source/data trees."""
import argparse, ctypes, hashlib, json, os, pathlib, subprocess, time
from PIL import Image

p = argparse.ArgumentParser()
p.add_argument('run', type=pathlib.Path)
p.add_argument('--platform', choices=['windows', 'linux'], required=True)
p.add_argument('--pid', type=int)
p.add_argument('--phase', default='menu')
p.add_argument('--scales', action='store_true')
a = p.parse_args()
game, evidence = a.run/'game', a.run/'evidence'
assert game.is_dir() and evidence.is_dir()
channel = game/'_harness_cmd.txt'
shots = a.run/'user-data'/'screenshots'

def command(text):
    deadline = time.monotonic()+300
    while channel.exists():
        if time.monotonic()>deadline: raise TimeoutError('previous game command')
        time.sleep(.1)
    log=game/'_saveload.log'
    offset=log.stat().st_size if log.exists() else 0
    temporary=channel.with_suffix('.tmp')
    temporary.write_bytes((text+'\n').encode('ascii'))
    temporary.replace(channel)
    while channel.exists():
        if time.monotonic()>deadline: raise TimeoutError(text)
        time.sleep(.1)
    while ('[harness] cmd: '+text) not in log.read_bytes()[offset:].decode('latin1'):
        if time.monotonic()>deadline: raise TimeoutError('command acknowledgement: '+text)
        time.sleep(.1)
    time.sleep(1)

if a.platform == 'windows':
    assert a.pid
    u = ctypes.windll.user32
    u.SetProcessDPIAware()
    found = []
    callback = ctypes.WINFUNCTYPE(ctypes.c_bool, ctypes.c_void_p, ctypes.c_void_p)
    def visit(hwnd, _):
        pid = ctypes.c_ulong()
        u.GetWindowThreadProcessId(ctypes.c_void_p(hwnd), ctypes.byref(pid))
        if pid.value == a.pid and u.IsWindowVisible(ctypes.c_void_p(hwnd)):
            found.append(hwnd)
        return True
    command('displaystatus')
    u.EnumWindows(callback(visit), 0)
    assert found
    hwnd = ctypes.c_void_p(found[0])
    class Rect(ctypes.Structure):
        _fields_ = [('left',ctypes.c_long),('top',ctypes.c_long),('right',ctypes.c_long),('bottom',ctypes.c_long)]
    def resize(w,h):
        if w>u.GetSystemMetrics(0)-16 or h>u.GetSystemMetrics(1)-80:
            command(f'console gfx_resolution {w}x{h}')
            command('console gfx_update')
            return
        r = Rect(0,0,w,h)
        style = u.GetWindowLongW(hwnd,-16)
        extended = u.GetWindowLongW(hwnd,-20)
        assert u.AdjustWindowRectEx(ctypes.byref(r),style,False,extended)
        assert u.SetWindowPos(hwnd,None,0,0,r.right-r.left,r.bottom-r.top,0x14)
else:
    command('displaystatus')
    windows = subprocess.check_output(['xdotool','search','--class','Game'],text=True).splitlines()
    assert len(windows)==1, windows
    def resize(w,h):
        subprocess.run(['xdotool','windowsize',windows[0],str(w),str(h)],check=True)

rows = []
def capture(w,h,scale):
    time.sleep(2)
    command('displaystatus')
    old = {f.name:f.stat().st_mtime_ns for f in shots.glob('*.bmp')}
    command('screenshot')
    deadline = time.monotonic()+300
    while True:
        files = [f for f in shots.glob('*.bmp') if f.name not in old or f.stat().st_mtime_ns>old[f.name]]
        if files: break
        if time.monotonic()>deadline: raise TimeoutError('frame readback')
        time.sleep(.1)
    src = max(files,key=lambda f:f.stat().st_mtime_ns)
    name = f'{a.phase}-{w}x{h}-scale-{scale}.png'
    output = evidence/name
    # The command channel is consumed before the emulated process finishes I/O.
    while True:
        try:
            with Image.open(src) as im:
                assert im.size == (w,h), (name,im.size)
                rgb = im.convert('RGB')
            rgb.save(output)
            break
        except (OSError, ValueError):
            if time.monotonic()>deadline: raise
            time.sleep(.2)
    row = dict(frame=name,width=w,height=h,scale=scale,sha256=hashlib.sha256(output.read_bytes()).hexdigest())
    rows.append(row)
    (evidence/f'{a.phase}-matrix.json').write_text(json.dumps(rows,indent=2)+'\n')
    print(json.dumps(row),flush=True)

command('console gfx_fullscreen 0')
command('console gfx_update')
command('console ui_vector_fonts 1')
command('console ui_scale 0')
for w,h in [(1024,768),(1280,720),(1920,1080),(3440,1440),(3840,2160)]:
    resize(w,h)
    capture(w,h,0)
if a.scales:
    resize(1920,1080)
    for scale in [75,100,125,150,200]:
        command(f'console ui_scale {scale}')
        capture(1920,1080,scale)
    command('console ui_scale 0')
print('Matrix complete',flush=True)
