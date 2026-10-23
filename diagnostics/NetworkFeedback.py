#!/usr/bin/env python3
"""Rendered multiplayer regression: real pointer events, both roles, combat and audio.

Requires an unpacked Windows game. Reports and screenshots are preserved; the
harness always requests a graceful exit from both of its own game processes.
"""
from pathlib import Path
import argparse, csv, json, os, re, shutil, statistics, subprocess, time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--package', type=Path, required=True)
parser.add_argument('--exe', type=Path, help='Candidate Game.exe with adjacent network-version.txt')
parser.add_argument('--output', type=Path, required=True, help='New report directory')
parser.add_argument('--port', type=int, default=17891)
parser.add_argument('--hd', type=int, choices=(0, 1), default=1)
parser.add_argument('--skip-preparation', action='store_true', help='Only encounter/FPS/audio checks')
parser.add_argument('--max-p99-ms', type=float, default=100)
parser.add_argument('--max-frame-ms', type=float, default=500)
parser.add_argument('--min-spectator-ratio', type=float, default=.8,
                    help='Spectator FPS / own-turn aiming FPS minimum; 0 disables')
args = parser.parse_args()
if os.name != 'nt':
    parser.error('Rendered tests require Windows')
if not 1 <= args.port <= 65535:
    parser.error('Port must be 1..65535')
SOURCE = args.package.resolve()
candidate = (args.exe or SOURCE / 'Game.exe').resolve()
root = args.output.resolve()
for needed in (SOURCE / 'game.db', SOURCE / 'res', candidate, candidate.parent / 'network-version.txt'):
    if not needed.exists():
        parser.error('Required file/directory missing: ' + str(needed))
if args.hd and not (SOURCE / 'res-hd').is_dir():
    parser.error('HD resources missing')
root.mkdir(parents=True, exist_ok=False)
BASE = root
processes = []
startup = subprocess.STARTUPINFO()
startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
startup.wShowWindow = 0

def command(role,text,timeout=30):
    root=BASE/role; path=root/'_harness_cmd.txt'
    end=time.monotonic()+timeout
    while path.exists() and time.monotonic()<end: time.sleep(.05)
    if path.exists(): raise RuntimeError(f'{role} did not consume previous command')
    temporary=root/'_harness_cmd.tmp'; temporary.write_text(text,encoding='ascii'); temporary.replace(path)
    while path.exists() and time.monotonic()<end: time.sleep(.05)
    if path.exists(): raise RuntimeError(f'{role} did not consume {text}')
    time.sleep(.15)
def status(role):
    command(role,'netstatus'); log=(BASE/role/'_saveload.log').read_text(errors='replace')
    matches=list(re.finditer(r'\[network\] local=(\d+) turn=(-?\d+) replica=(\d+) time=(\d+) executing=(\d+)',log))
    if not matches or log.rfind('[network] no active match')>matches[-1].start(): return None
    last=matches[-1]; result=dict(zip(('local','turn','replica','time','executing'),map(int,last.groups())))
    result['units']=[dict(zip(('index','ap','ammo','pose','command','x','y','z'),map(float,m.groups()))) for m in re.finditer(r'unit=(\d+) ap=(-?\d+) ammo=(-?\d+) pose=(\d+) command=(\d+) pos=([\d.-]+),([\d.-]+),([\d.-]+)',log[last.end():])]
    result['teams']=[dict(zip(('side','index','alive','x','y','z'),map(float,m.groups()))) for m in re.finditer(r'team=(\d+) fighter=(\d+) alive=(\d+) pos=([\d.-]+),([\d.-]+),([\d.-]+)',log[last.end():])]
    for unit in result['units']:
        extra=re.search(r'unit='+str(int(unit['index']))+r' .*?hp=(-?\d+) grid=(\d+),(\d+) mode=(-?\d+)',log[last.end():])
        if extra:unit.update(dict(zip(('hp','gx','gy','mode'),map(int,extra.groups()))))
    return result

def cmd(role,value):command(role,value,timeout=90)
def log(role):return (root/role/'_saveload.log').read_text(errors='replace')
def wait(predicate,timeout=120):
    end=time.monotonic()+timeout
    while time.monotonic()<end:
        if predicate():return
        if any(p.poll() is not None for p in processes):raise RuntimeError('Game exited')
        time.sleep(.25)
    raise TimeoutError('Condition timed out')
def tree(role):cmd(role,'displaytree');return log(role).rsplit('[harness] cmd: displaytree',1)[-1]
def control(role,name):
    text=tree(role);match=re.search(r'\[display-control\].*?id='+re.escape(name)+r' position=.*?clip=(-?\d+),(-?\d+),(-?\d+),(-?\d+)',text)
    if not match:raise RuntimeError('Missing visible control '+role+' '+name)
    rect=list(map(int,match.groups()));cmd(role,'uipoint %d %d'%((rect[0]+rect[2])//2,(rect[1]+rect[3])//2))
def value(role,name):
    cmd(role,'uiinspect '+name);m=re.findall(r'\[ui-text\] id='+re.escape(name)+r' active=\d value=([0-9a-f]*)',log(role))
    if not m:raise RuntimeError('Text not reported '+name)
    return bytes.fromhex(m[-1]).decode('utf-16-be')
def screenshot(role,tag):
    cmd(role,'screenshot');time.sleep(.3)
    images=sorted((root/(role+'-user-data/screenshots')).glob('*.bmp'),key=lambda p:p.stat().st_mtime_ns)
    shutil.copy2(images[-1],root/(role+'-'+tag+'.bmp'))
def capture(tag,frames=400):
    for role in ('host','client'):cmd(role,'perf '+tag+' '+str(frames))
    wait(lambda:all((root/role/('_perf_'+tag+'.csv')).exists() and len((root/role/('_perf_'+tag+'.csv')).read_text().splitlines())>=frames+1 for role in ('host','client')))
def camera(role):
    state=status(role);matches=re.findall(r'\[network-camera\] center=([\d.-]+),([\d.-]+),([\d.-]+)',log(role));point=list(map(float,matches[-1]))
    unit=state['units'][0]
    assert (point[0]-unit['x'])**2+(point[1]-unit['y'])**2<9,(role,'wrong camera',point,unit)
def audio(role):
    cmd(role,'netaudio');m=re.findall(r'playing3d=(\d+) looping3d=(\d+)',log(role));return list(map(int,m[-1]))
def aim(role):
    state=status(role);opponents=[u for u in state['teams'] if u['side']!=state['local']]
    for unit in opponents:
        for height in [1.1,.8,1.5]:
            cmd(role,f"projectpoint {unit['x']} {unit['y']} {unit['z']+height}")
            part=log(role).rsplit('[harness] cmd: projectpoint',1)[-1];m=re.search(r'visible=1 pixel=([\d.-]+),([\d.-]+)',part)
            if not m:continue
            x,y=[int(float(v)) for v in m.groups()]
            if not 60<y<735 or not 0<x<1275:continue
            cmd(role,f'uihover {x} {y}');time.sleep(.6);cmd(role,'netstaff')
            state=log(role).rsplit('[harness] cmd: netstaff',1)[-1]
            if 'CStateAttack' in state:print(role,'aimed',unit['index'],x,y,flush=True);return True
    return False
def check_stop_resume():
    report={}
    for side,role in enumerate(('host','client')):
        cmd('host',f'nettestturn {side}');time.sleep(.5)
        for dx,dy in ((12,0),(0,12),(-12,0),(0,-12)):
            cmd(role,f'netmove 0 {dx} {dy}')
            end=time.monotonic()+4
            while time.monotonic()<end:
                state=status(role)
                if state['executing'] and state['units'][0]['command']:break
                time.sleep(.1)
            if state['executing'] and state['units'][0]['command']:break
        else:raise RuntimeError(role+' has no valid stop/resume route')
        cmd(role,'netcancel');wait(lambda:not status(role)['executing'],12)
        paused=status(role)['units'][0];time.sleep(.7)
        assert status(role)['units'][0]['command']==1,'Stop erased the pending order'
        # Exhausting AP can select the next fighter. Select this fighter's tab.
        tab=tree(role);match=re.search(r'id=unitstabbar position=.*?clip=(\d+),(\d+),(\d+),(\d+)',tab)
        assert match,'Squad tabs missing'
        x1,y1,x2,y2=map(int,match.groups())
        # The tab is an action decorator: let its hover target update before
        # delivering mouse-down, as in normal input on separate game frames.
        cmd(role,f'uihover {x1+60} {(y1+y2)//2}');time.sleep(.4)
        cmd(role,f'uipoint {x1+60} {(y1+y2)//2}')
        def available():
            cmd(role,'netui')
            values=re.findall(r'\[network-ui\] action=2 available=(\d) ok=(\d)',log(role))
            return values and values[-1]==('1','1')
        wait(available,10);screenshot(role,'continue-enabled');before=status(role)['units'][0]
        control(role,'icon_2')
        wait(lambda:(lambda u:(u['gx'],u['gy'])!=(before['gx'],before['gy']))(status(role)['units'][0]),12)
        after=status(role)['units'][0]
        assert 0<=after['ap']<before['ap'],'Continue exceeded or did not spend AP'
        report[role]={'paused':paused,'before':before,'after':after}
        cmd(role,'netcancel');wait(lambda:not status(role)['executing'],12)
    cmd('host','nettestturn 0');time.sleep(.5)
    (root/'stop-resume-report.json').write_text(json.dumps(report,indent=2))
    print('STOP/CONTINUE real mouse click passed for both roles',flush=True)

def check_combat():
    report={'visibility':{},'aim':{},'bursts':[]}
    for role in ['host','client']:
        cmd(role,'netvisible');part=log(role).rsplit('[harness] cmd: netvisible',1)[-1];m=re.search(r'enemies=(\d+)',part);report['visibility'][role]=int(m[1]);print(role,part.strip(),flush=True)
        cmd(role,'netselectall');cmd(role,'netui')
        assert '[network-ui] selected=6' in log(role).rsplit('[harness] cmd: netui',1)[-1]
    for side,role in enumerate(['host','client']):
        cmd('host',f'nettestturn {side}');time.sleep(3);cmd(role,'netfocus 1');report['aim'][role]=aim(role)
        capture(role+'_all_aim',600);cmd(role,'screenshot')
    assert all(n == 6 for n in report['visibility'].values()), 'Fixture must expose six opponents to each role'
    assert all(report['aim'].values()), 'Mouse must enter native attack state for both roles'
    print('ALL-SELECTED AIM CAPTURED',report,flush=True)
    state=status('client');weapons=re.findall(r'\[network-weapon\] unit=(\d+) db=(\d+) longburst=(\d+)',log('client').rsplit('[network] local=',1)[-1]);choices=[int(i) for i,db,long in weapons if long=='1' and state['units'][int(i)]['ammo']>=8]
    assert choices, 'Fixture has no loaded long-burst weapon'
    for actor in choices:
        cmd('host','nettestturn 1');time.sleep(.5);cmd('client',f'netmode {actor} 4');time.sleep(.7)
        cmd('host',f'nettestap 1 {actor} 19');time.sleep(.3)
        before=status('client')['units'][actor];baseline={r:audio(r) for r in ['host','client']}
        for role in ['host','client']:cmd(role,f'perf burst_actor_{actor} 500')
        cmd('client',f'netshoot {actor} 0 5');observations=[];start=time.monotonic()
        while time.monotonic()-start<9:
            sample=status('client');unit=sample['units'][actor]
            observations.append({'seconds':round(time.monotonic()-start,2),'ap':unit['ap'],'ammo':unit['ammo'],'executing':sample['executing'],'audio':{r:audio(r) for r in ['host','client']}});time.sleep(.2)
        after=status('client')['units'][actor]
        result={'actor':actor,'before':before,'after':after,'baseline':baseline,'observations':observations};report['bursts'].append(result)
        print('BURST',actor,'ammo',before['ammo'],after['ammo'],'AP',after['ap'],'end-audio',observations[-1]['audio'],flush=True)
        assert after['ammo']<before['ammo'],'Burst did not fire'
        assert all(audio(r)[1]==0 for r in ['host','client']),'Loop persisted after action'
        wait(lambda:all(len((root/r/f'_perf_burst_actor_{actor}.csv').read_text().splitlines())>=501 for r in ('host','client')))
    # Exercise the unit-target attack path as well as shooting an empty tile.
    cmd('host','nettestturn 1');time.sleep(.5)
    cmd('host','nettestap 1 0 19');time.sleep(.5)
    before=status('client')['units'][0]
    cmd('client','netattack 0 0')
    wait(lambda:status('client')['units'][0]['ammo']<before['ammo'],5)
    wait(lambda:not status('client')['executing'],10)
    wait(lambda:all(audio(r)[1]==0 for r in ('host','client')),5)
    report['target_burst']={'before':before,'after':status('client')['units'][0],
                            'audio':{r:audio(r) for r in ('host','client')}}
    (root/'stress-report.json').write_text(json.dumps(report,indent=2))
    print('STRESS AP exhaustion and target shooting passed',flush=True)
try:
    for role in ('host','client'):
        game=root/role;game.mkdir()
        for p in SOURCE.iterdir():
            if p.is_file() and p.suffix.lower() in ('.exe','.dll','.db','.mma','.cfg'):shutil.copy2(p,game/p.name)
        for name in ('cfg','fonts','scripts'):shutil.copytree(SOURCE/name,game/name)
        for name in ('res','res-hd'):
            if not (SOURCE/name).exists():continue
            subprocess.run(['cmd','/c','mklink','/J',str(game/name),str(SOURCE/name)],check=True,stdout=subprocess.DEVNULL)
        shutil.copy2(candidate,game/'Game.exe')
        shutil.copy2(candidate.parent/'network-version.txt',game/'network-version.txt')
        (game/'bench.cfg').write_text('setvar gfx_hd_textures = '+str(args.hd)+'\nmainmenu\n',encoding='ascii')
        p=subprocess.Popen([str(game/'Game.exe'),'-harness','-harness-active','-windowed','-cfg','bench.cfg'],cwd=game,env=dict(os.environ,S2_USER_DATA_DIR=str(root/(role+'-user-data'))),startupinfo=startup,stdout=(root/(role+'-output.log')).open('w'),stderr=subprocess.STDOUT)
        processes.append(p);(root/(role+'.pid')).write_text(str(p.pid));print(role,p.pid,flush=True)
    wait(lambda:all((root/(r+'-user-data/startup.log')).exists() and 'main-menu-presented' in (root/(r+'-user-data/startup.log')).read_text() for r in ('host','client')))
    for role in ('host','client'):cmd(role,'netmenu')
    if args.skip_preparation:
        control('host','net_preparation');cmd('host','uiset net_port '+str(args.port)+'');cmd('client','uiset net_port '+str(args.port)+'')
        control('host','net_create');control('client','net_connect');wait(lambda:all(status(r) for r in ('host','client')),180)
    else:
        control('host','net_squad_size');cmd('host','uikey 36');cmd('host','uikey 46');cmd('host','uitype 3')
        assert value('host','net_squad_size')=='3','Mouse-focused numeric typing failed'
        for i in range(3):control('host','net_squad_size_more')
        assert value('host','net_squad_size')=='6'
        control('host','net_squad_size_more');assert value('host','net_squad_size')=='6'
        screenshot('host','network-form')
        for key,text in [('net_supply_level','14'),('net_starting_level','5'),('net_port',''+str(args.port)+'')]:cmd('host','uiset '+key+' '+text)
        cmd('client','uiset net_port '+str(args.port)+'');control('host','net_create');control('client','net_connect')
        wait(lambda:all(status(r) for r in ('host','client')),180)
        for role in ('host','client'):
            time.sleep(2);control(role,'net_base_roster');before=value(role,'net_recruit_0');control(role,'net_recruit_next_0')
            wait(lambda:(lambda text:bool(text) and text!=before)(value(role,'net_recruit_0')),90)
            assert 'id=net_recruit_next_0 ' in tree(role),'Roster closed after replacement'
            started=status(role)['time']
            for edit in range(12):
                index=edit%6;before=value(role,'net_recruit_'+str(index));control(role,'net_recruit_next_'+str(index))
                wait(lambda:value(role,'net_recruit_'+str(index))!=before)
            state=status(role)
            personas=re.findall(r'\[network-persona\] unit=(\d+) record=(\d+) gameplay=(\d+)',log(role).rsplit('[harness] cmd: netstatus\n',1)[-1])
            assert len(state['units'])==6 and len(personas)==6 and len({p[1] for p in personas})==6,'Roster lost or duplicated a fighter'
            assert state['time']>started,'Recruit replacement reset the base clock'
            screenshot(role,'roster');control(role,'net_base_roster')
            control(role,'net_base_store');control(role,'heavyweapon');screenshot(role,'store-heavy-top')
            for y in (200,350,500,550,400,200,110):cmd(role,f'uipoint 590 {y}')
            screenshot(role,'store-heavy-scrolled');control(role,'store')
            control(role,'net_base_ready');wait(lambda:'\u0412\u044b \u0433\u043e\u0442\u043e\u0432\u044b' in value(role,'net_base_status'))
            assert '\u041e\u0442\u043c\u0435\u043d\u0438\u0442\u044c' in value(role,'net_base_ready')
            screenshot(role,'ready');control(role,'net_base_ready')
            wait(lambda:'\u0412\u044b \u0433\u043e\u0442\u043e\u0432\u044b' not in value(role,'net_base_status'))
        print('PREPARATION mouse/typing/steppers/repeated-unique-recruits/store/ready passed',flush=True)
        for role in ('host','client'):control(role,'net_base_ready')
        wait(lambda:all('id=net_base_ready ' not in tree(r) for r in ('host','client')),180)
        time.sleep(3)
        for role in ('host','client'):camera(role);screenshot(role,'battle-spawn')
        for role,other in [('host','client'),('client','host')]:
            before=status(other)['time']
            control(role,'gamemenu');time.sleep(2)
            assert 'id=net_match_resume ' in tree(role) and status(other)['time']>before,'Menu stopped/disconnected match'
            screenshot(role,'match-menu');control(role,'net_match_resume');assert status(other)
        control('host','gamemenu');control('host','net_match_leave');assert status('client'),'First exit click disconnected'
        control('host','net_match_leave');wait(lambda:all(not status(r) for r in ('host','client')))
        print('CAMERA and MENU resume/confirmed-exit passed',flush=True)
        for role in ('host','client'):cmd(role,'netmenu')
        control('host','net_preparation');cmd('host','uiset net_port '+str(args.port)+'');cmd('client','uiset net_port '+str(args.port)+'')
        control('host','net_create');control('client','net_connect');wait(lambda:all(status(r) for r in ('host','client')),180)
    time.sleep(4);check_stop_resume();capture('separated')
    base=status('host')['units'][0];gx,gy=int(base['gx']),int(base['gy']);placed=[]
    for side in (0,1):
        for index in range(6):
            for attempt in range(40):
                x=gx+8+side*8+(index%3)*2+(attempt//8)*2;y=gy+8+(index//3)*2+(attempt%8)*2
                if (x,y) in placed:continue
                cmd('host',f'nettestplace {side} {index} {x} {y}')
                if '[network-fixture] placed' in log('host').rsplit('[harness] cmd: nettestplace',1)[-1]:placed.append((x,y));break
            else:raise RuntimeError('Could not place encounter fixture')
    time.sleep(3)
    for role in ('host','client'):cmd(role,'netfocus 1');cmd(role,'netvisible');screenshot(role,'encounter')
    capture('encounter',800)
    print('ENCOUNTER captured, twelve nearby units',flush=True)
    check_combat()
finally:
    errors=[]
    for role,p in zip(('host','client'),processes):
        if p.poll() is None:
            try:cmd(role,'quit');p.wait(timeout=60)
            except Exception as error:errors.append(role+' PID '+str(p.pid)+': '+str(error))
    if errors:raise RuntimeError('Test games did not exit gracefully: '+', '.join(errors))
summary={}
for role in ('host','client'):
    for path in (root/role).glob('_perf_*.csv'):
        rows=list(csv.DictReader(path.open()));v=sorted(float(r['frame_interval_ms']) for r in rows if float(r['frame_interval_ms'])>0)
        summary[role+'/'+path.stem]=dict(fps=round(1000/statistics.mean(v),2),p99=round(v[int((len(v)-1)*.99)],3),max=max(v),over100=sum(x>100 for x in v),scopes={k:round(statistics.mean(float(r[k]) for r in rows),3) for k in rows[0] if k.endswith('_ms')})
(root/'summary.json').write_text(json.dumps(summary,indent=2));print(json.dumps(summary,indent=2))

failures=[]
for name,metrics in summary.items():
    if args.max_p99_ms>0 and metrics['p99']>args.max_p99_ms:failures.append(name+' p99')
    if args.max_frame_ms>0 and metrics['max']>args.max_frame_ms:failures.append(name+' maximum frame')
for role,other in [('host','client'),('client','host')]:
    ratio=summary[role+'/_perf_'+other+'_all_aim']['fps']/summary[role+'/_perf_'+role+'_all_aim']['fps']
    if args.min_spectator_ratio>0 and ratio<args.min_spectator_ratio:failures.append(role+' spectator FPS ratio '+str(ratio))
if failures:raise RuntimeError('Frame-time regression: '+', '.join(failures))
(root/'PASSED.txt').write_text(
    ('Preparation/menu/camera skipped; ' if args.skip_preparation else
     'UI, squad replacement, ready, camera and safe menu passed; ') +
    'encounter FPS and burst audio passed\n')
