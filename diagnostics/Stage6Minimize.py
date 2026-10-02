"""Check zero-sized drawable suspension with an ordinary OS window operation."""
import argparse, ctypes, pathlib, re, subprocess, time

p = argparse.ArgumentParser()
p.add_argument('run', type=pathlib.Path)
p.add_argument('--platform', choices=['windows', 'linux'], required=True)
p.add_argument('--target', required=True)
a = p.parse_args()
channel = a.run/'game/_harness_cmd.txt'
log = a.run/'game/_saveload.log'

def status():
    deadline = time.monotonic()+300
    while channel.exists():
        if time.monotonic()>deadline: raise TimeoutError('busy harness')
        time.sleep(.1)
    offset = log.stat().st_size
    temporary = channel.with_suffix('.tmp')
    temporary.write_bytes(b'displaystatus\n')
    temporary.replace(channel)
    while True:
        text = log.read_bytes()[offset:].decode('latin1')
        matches = re.findall(r'\[display\].*drawable=(\d).*', text)
        if matches and not channel.exists():
            print(text, flush=True)
            return int(matches[-1])
        if time.monotonic()>deadline: raise TimeoutError('displaystatus')
        time.sleep(.1)

if a.platform == 'windows':
    user = ctypes.windll.user32
    window = None
    callback_type = ctypes.WINFUNCTYPE(ctypes.c_bool, ctypes.c_void_p, ctypes.c_void_p)
    def find(hwnd, _):
        global window
        pid = ctypes.c_ulong()
        user.GetWindowThreadProcessId(ctypes.c_void_p(hwnd), ctypes.byref(pid))
        if pid.value == int(a.target) and user.IsWindowVisible(ctypes.c_void_p(hwnd)):
            window = hwnd
            return False
        return True
    callback = callback_type(find)
    user.EnumWindows(callback, 0)
    assert window, 'No visible game window'
    def minimize(): user.ShowWindow(ctypes.c_void_p(window), 6)
    def restore(): user.ShowWindow(ctypes.c_void_p(window), 9)
else:
    def minimize(): subprocess.run(['xdotool', 'windowminimize', a.target], check=True)
    def restore():
        subprocess.run(['xdotool', 'windowmap', a.target], check=True)
        subprocess.run(['xdotool', 'windowactivate', '--sync', a.target], check=True)

assert status() == 1
try:
    minimize()
    deadline = time.monotonic()+120
    while status() != 0:
        assert time.monotonic()<deadline, 'Minimized surface must suspend rendering'
        time.sleep(1)
finally:
    restore()
deadline = time.monotonic()+120
while status() != 1:
    assert time.monotonic()<deadline, 'Restored surface must resume rendering'
    time.sleep(1)
print('Minimize/restore passed', flush=True)
