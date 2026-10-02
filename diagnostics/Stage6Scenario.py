"""Replay graphical checks; clicks/keys/drags go through ordinary OS input."""
import argparse, json, os, pathlib, re, subprocess, sys, time
from PIL import Image

p=argparse.ArgumentParser()
p.add_argument('run',type=pathlib.Path)
p.add_argument('steps',type=pathlib.Path)
p.add_argument('--platform',choices=['windows','linux'],required=True)
p.add_argument('--target',required=True)
p.add_argument('--helper')
p.add_argument('--capture-timeout',type=float,default=60,help='Allow slow emulated GPU readback without altering gameplay')
a=p.parse_args()
channel=a.run/'game/_harness_cmd.txt'
log=a.run/'game/_saveload.log'
evidence=a.run/'evidence'

def command(value):
    deadline=time.monotonic()+300
    while channel.exists():
        if time.monotonic()>deadline: raise TimeoutError('busy game')
        time.sleep(.1)
    offset=log.stat().st_size if log.exists() else 0
    temporary=channel.with_suffix('.tmp')
    temporary.write_bytes((value+'\n').encode('ascii'))
    temporary.replace(channel)
    while channel.exists():
        if time.monotonic()>deadline: raise TimeoutError(value)
        time.sleep(.1)
    while ('[harness] cmd: '+value) not in log.read_bytes()[offset:].decode('latin1'):
        if time.monotonic()>deadline: raise TimeoutError('command acknowledgement: '+value)
        time.sleep(.1)
    time.sleep(.5)
    return log.read_bytes()[offset:].decode('latin1')

def input_action(action,*values):
    args=[sys.executable,str(pathlib.Path(__file__).with_name('Stage6Input.py')),str(a.run),
          '--platform',a.platform,'--target',a.target]
    if a.helper: args+=['--helper',a.helper]
    subprocess.run(args+[action,*map(str,values)],check=True)

def control(suffix, action="click", at=(.5,.5)):
    text=command('displaytree')
    stack=[]
    nodes=[]
    for depth,id,x1,y1,x2,y2 in re.findall(r'depth=(\d+) id=(.*?) position=-?\d+,-?\d+ clip=(-?\d+),(-?\d+),(-?\d+),(-?\d+)',text):
        depth=int(depth)
        stack=stack[:depth]+[id]
        path='/'.join(stack)
        if path.endswith(suffix): nodes.append((path,*map(int,[x1,y1,x2,y2])))
    assert len(nodes)==1,(suffix,nodes,text)
    path,x1,y1,x2,y2=nodes[0]
    print('visible control',path,(x1,y1,x2,y2),flush=True)
    input_action(action,x1+(x2-x1)*at[0],y1+(y2-y1)*at[1])

remembered = {}
for step in json.loads(a.steps.read_text()):
    print('step',step,flush=True)
    if 'command' in step: print(command(step['command']),flush=True)
    elif 'wait' in step: time.sleep(step['wait'])
    elif 'sector' in step:
        result=command('displaytree')
        rows=re.findall(r'\[chapter-sector\] index=(\d+) type=(\d+) template=(-?\d+) visible=(\d+) hit=(\d+) local=(-?\d+),(-?\d+)',result)
        options=step['sector']
        choices=[tuple(map(int,row)) for row in rows if all(int(row[col])==options[key] for key,col in [('index',0),('type',1),('template',2),('visible',3)] if key in options)]
        assert choices,('no matching chapter marker',options,result)
        chosen=choices[0]
        parent=re.search(r'depth=2 id=view position=(-?\d+),(-?\d+)',result)
        assert parent,('chapter viewport missing',result)
        metrics=command('displaystatus')
        scale=float(re.search(r' scale=([\d.]+)',metrics).group(1))
        size=49 if chosen[1]==1 else 69
        origin=tuple(map(int,parent.groups()))
        print('chapter marker',chosen,origin,scale,flush=True)
        input_action('click',origin[0]+(chosen[5]+size/2)*scale,origin[1]+(chosen[6]+size/2)*scale)
    elif 'advance_dialogue' in step:
        deadline=time.monotonic()+step.get('timeout',180)
        while True:
            result=command('displaytree')
            if re.search(r'id=missionUI ',result): break
            assert re.search(r'id=dialogUI ',result),('unexpected screen',result)
            if time.monotonic()>deadline: raise TimeoutError('dialogue')
            control('dialogUI/next' if re.search(r'depth=2 id=next ',result) else 'dialogUI/cancel')
            time.sleep(1)
    elif 'wait_travel' in step:
        deadline=time.monotonic()+step.get('timeout',180)
        while True:
            result=command('displaytree')
            match=re.search(r'\[chapter\] position=([-\d.]+),([-\d.]+) target=([-\d.]+),([-\d.]+)',result)
            if not match: raise AssertionError(('chapter screen unavailable',result))
            x,y,tx,ty=map(float,match.groups())
            if (x-tx)**2+(y-ty)**2<=1.1: break
            if time.monotonic()>deadline: raise TimeoutError(('travel',x,y,tx,ty))
            time.sleep(.5)
    elif 'wait_until' in step or 'wait_not' in step:
        deadline=time.monotonic()+step.get('timeout',300)
        while True:
            result=command(step.get('query','displaytree'))
            matched=bool(re.search(step.get('wait_until',step.get('wait_not')),result))
            if matched == ('wait_until' in step): break
            if time.monotonic()>deadline: raise TimeoutError(step)
            time.sleep(.5)
    elif 'control' in step: control(step['control'],at=step.get('at',(.5,.5)))
    elif 'hover' in step: control(step['hover'],'move',step.get('at',(.5,.5)))
    elif 'input' in step: input_action(*step['input'])
    elif 'capture' in step:
        old={f.name:f.stat().st_mtime_ns for f in (a.run/'user-data/screenshots').glob('*.bmp')}
        command(step.get('capture_command','screenshot'))
        deadline=time.monotonic()+a.capture_timeout
        while True:
            files=[f for f in (a.run/'user-data/screenshots').glob('*.bmp') if f.name not in old or f.stat().st_mtime_ns>old[f.name]]
            if files: break
            if time.monotonic()>deadline: raise TimeoutError('screenshot')
            time.sleep(.1)
        src=max(files,key=lambda f:f.stat().st_mtime_ns)
        output=evidence/(step['capture']+'.png')
        while True:
            try:
                with Image.open(src) as im: rgb=im.convert('RGB')
                rgb.save(output)
                break
            except (OSError,ValueError):
                if time.monotonic()>deadline: raise
                time.sleep(.2)
        print('frame',output,flush=True)
    elif 'remember' in step or 'assert_remember' in step:
        result=command(step.get('query','displaytree'))
        match=re.search(step['pattern'],result)
        assert match,(step,result)
        values=match.groups()
        if 'remember' in step: remembered[step['remember']]=values
        else: assert values==remembered[step['assert_remember']],(values,remembered,step)
    elif 'assert' in step:
        result=command(step.get('query','displaystatus'))
        assert re.search(step['assert'],result),(step,result)
    else: raise ValueError(step)
print('Scenario complete',flush=True)
