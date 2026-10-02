"""Ordinary OS mouse/keyboard input; the game harness only reports cursor/geometry."""
import argparse, ctypes, pathlib, re, subprocess, time
p=argparse.ArgumentParser()
p.add_argument('run',type=pathlib.Path)
p.add_argument('--platform',choices=['windows','linux'],required=True)
p.add_argument('--target',required=True)
p.add_argument('--helper')
p.add_argument('action',choices=['click','move','key','ldrag'])
p.add_argument('values',nargs='*')
a=p.parse_args()
if a.platform == 'linux':
    x11 = ctypes.CDLL('libX11.so.6')
    xtst = ctypes.CDLL('libXtst.so.6')
    x11.XOpenDisplay.restype = ctypes.c_void_p
    x11.XOpenDisplay.argtypes = [ctypes.c_char_p]
    x11.XFlush.argtypes = [ctypes.c_void_p]
    xtst.XTestFakeRelativeMotionEvent.argtypes = [ctypes.c_void_p,ctypes.c_int,ctypes.c_int,ctypes.c_ulong]
    connection = x11.XOpenDisplay(None)
    assert connection, 'X11 display unavailable'
    def relative(dx,dy):
        assert xtst.XTestFakeRelativeMotionEvent(connection,int(dx),int(dy),0)
        x11.XFlush(connection)
channel=a.run/'game'/'_harness_cmd.txt'
log=a.run/'game'/'_saveload.log'
assert channel.parent.is_dir() and log.is_file()
def command(text):
    deadline=time.monotonic()+300
    while channel.exists():
        if time.monotonic()>deadline: raise TimeoutError('busy game')
        time.sleep(.1)
    offset=log.stat().st_size
    temporary=channel.with_suffix('.tmp')
    temporary.write_bytes((text+'\n').encode('ascii'))
    temporary.replace(channel)
    while channel.exists():
        if time.monotonic()>deadline: raise TimeoutError(text)
        time.sleep(.1)
    while ('[harness] cmd: '+text) not in log.read_bytes()[offset:].decode('latin1'):
        if time.monotonic()>deadline: raise TimeoutError('command acknowledgement: '+text)
        time.sleep(.1)
    time.sleep(.3)
def send(action,*values):
    if a.platform=='windows':
        # Foreground can change between cursor movement and a click. Keep the
        # helper's foreground guard and retry only its explicit 'no input sent'.
        for attempt in range(3):
            result=subprocess.run([a.helper,a.target,action,*map(str,values)])
            if result.returncode != 4 or attempt == 2:
                result.check_returncode()
                break
            time.sleep(.5)
    else:
        wm = subprocess.check_output(['xprop','-root','_NET_SUPPORTING_WM_CHECK'], text=True)
        focus = 'windowactivate' if 'window id #' in wm else 'windowfocus'
        subprocess.run(['xdotool',focus,'--sync',a.target],check=True)
        if action=='move':
            relative(*values)
            command('displaystatus')
            command('displaystatus')
            return
        elif action=='click':
            subprocess.run(['xdotool','mousedown','1'],check=True)
            # A physical press spans input frames, including slow QEMU rendering.
            command('displaystatus')
            command('displaystatus')
            cmd=['mouseup','1']
        elif action=='key': cmd=['key',values[0]]
        else:
            subprocess.run(['xdotool','mousedown','1'],check=True)
            time.sleep(.2)
            relative(*values)
            time.sleep(.5)
            cmd=['mouseup','1']
        subprocess.run(['xdotool',*cmd],check=True)
        command('displaystatus')
        command('displaystatus')
    time.sleep(.3)
def state():
    command('displaystatus')
    text=log.read_text(errors='replace')
    metrics=re.findall(r'\[display\] window=(\d+)x(\d+) pixels=(\d+)x(\d+) dpi=([\d.]+) scale=([\d.]+) canvas=([\d.]+)x([\d.]+).*confirmation=(\d+)',text)[-1]
    cursor=re.findall(r'cursorUI=(\d+),(\d+)',text)[-1]
    print('state',metrics,'cursor',cursor,flush=True)
    return tuple(map(float,metrics)),tuple(map(float,cursor))
if a.action in ['move','click']:
    x,y=map(float,a.values)
    previous = None
    gain = [2.0,2.0]
    for _ in range(24):
        metrics,cursor=state()
        scale=metrics[5]
        position = [cursor[0]*scale,cursor[1]*scale]
        if previous:
            old, movement = previous
            for axis in range(2):
                if abs(movement[axis]) >= 4:
                    observed = (position[axis]-old[axis])/movement[axis]
                    if .5 <= observed <= 6: gain[axis] = observed
        dx,dy=x-position[0],y-position[1]
        if abs(dx)<=3 and abs(dy)<=3: break
        # Measure retained acceleration through visible cursor feedback.
        delta=lambda d,g: max(-200,min(200,round(d/g))) or (1 if d>0 else -1) if abs(d)>3 else 0
        movement = [delta(dx,gain[0]),delta(dy,gain[1])]
        previous = (position,movement)
        send('move',*movement)
    else: raise RuntimeError('cursor did not reach the visible target')
    if a.action=='click': send('click')
elif a.action=='key': send('key',a.values[0])
else: send('ldrag',*map(int,a.values))
state()
print('ordinary input complete',flush=True)
