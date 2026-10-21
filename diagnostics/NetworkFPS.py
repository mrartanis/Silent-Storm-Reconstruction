#!/usr/bin/env python3
"""Repeatable rendered performance regression check. Use --help for arguments."""
from pathlib import Path
import os,sys,time,subprocess,csv,json,statistics,shutil
# The developer harness exercises the actual UI event and rendering path.
def command(role,value,timeout=90):
 game=root/role;path=game/'_harness_cmd.txt';end=time.monotonic()+timeout
 while path.exists() and time.monotonic()<end:time.sleep(.05)
 if path.exists():raise TimeoutError(role+' did not consume previous command')
 temporary=game/'_harness_cmd.tmp';temporary.write_text(value,encoding='ascii');temporary.replace(path)
 while path.exists() and time.monotonic()<end:time.sleep(.05)
 if path.exists():raise TimeoutError(role+' did not consume '+value)
 time.sleep(.15)
def active(role):
 command(role,'netstatus')
 text=(root/role/'_saveload.log').read_text(errors='replace')
 return text.rfind('[network] local=')>text.rfind('[network] no active match')
import argparse
parser=argparse.ArgumentParser(description='Two real Windows games, localhost base and CSV frame-time report; closes both test games.')
parser.add_argument('--package',type=Path,required=True,help='Unpacked full game with game.db, res, cfg, fonts, scripts and DLLs')
parser.add_argument('--exe',type=Path,help='Candidate Game.exe; its network-version.txt is copied too')
parser.add_argument('--output',type=Path,required=True,help='Fresh report directory (must not exist)')
parser.add_argument('--squad',type=int,choices=range(1,7),default=6)
parser.add_argument('--hd',type=int,choices=(0,1),default=1)
parser.add_argument('--idle-frames',type=int,default=1500)
parser.add_argument('--frames',type=int,default=6000)
parser.add_argument('--cycles',type=int,choices=range(1,101),default=12)
parser.add_argument('--port',type=int,default=17881)
parser.add_argument('--max-p99-ms',type=float,default=100,help='Fail if any capture exceeds this p99 frame interval; 0 disables the guard')
parser.add_argument('--max-frame-ms',type=float,default=500,help='Fail on a single longer frame; 0 disables the guard')
args=parser.parse_args()
if os.name!='nt':parser.error('This rendered benchmark currently supports Windows only')
if not all(100<=n<=10000 for n in (args.idle_frames,args.frames)):parser.error('Frame counts must be 100..10000')
if not 1<=args.port<=65535:parser.error('Port must be 1..65535')
SOURCE=args.package.resolve();root=args.output.resolve();count=args.squad;hd=args.hd
candidate=(args.exe or SOURCE/'Game.exe').resolve()
for needed in (SOURCE/'game.db',SOURCE/'res',candidate,candidate.parent/'network-version.txt'):
 if not needed.exists():parser.error('Required file/directory missing: '+str(needed))
if hd and not (SOURCE/'res-hd').is_dir():parser.error('HD resources missing')
root.mkdir(parents=True,exist_ok=False)
processes=[]
startup=subprocess.STARTUPINFO();startup.dwFlags|=subprocess.STARTF_USESHOWWINDOW;startup.wShowWindow=0
def wait(condition,seconds=100):
 end=time.monotonic()+seconds
 while time.monotonic()<end:
  if condition():return
  for p in processes:
   if p.poll() is not None:raise RuntimeError('Game exited '+str(p.returncode))
  time.sleep(.25)
 raise TimeoutError('Benchmark timed out')
def cmd(role,value):command(role,value,timeout=90)
try:
 for role in ('host','client'):
  game=root/role;game.mkdir()
  for p in SOURCE.iterdir():
   if p.is_file() and p.suffix.lower() in ('.exe','.dll','.db','.mma','.cfg'):shutil.copy2(p,game/p.name)
  for name in ('cfg','fonts','scripts'):shutil.copytree(SOURCE/name,game/name)
  for name in ('res','res-hd'):
   if (SOURCE/name).exists():subprocess.run(['cmd','/c','mklink','/J',str(game/name),str(SOURCE/name)],check=True,stdout=subprocess.DEVNULL)
  shutil.copy2(candidate,game/'Game.exe')
  shutil.copy2(candidate.parent/'network-version.txt',game/'network-version.txt')
  (game/'bench.cfg').write_text(('setvar gfx_hd_textures = '+str(hd)+'\nmainmenu\n'),encoding='ascii')
  profile=root/(role+'-user-data')
  env=dict(os.environ,S2_USER_DATA_DIR=str(profile),S2_TEXTURE_DIAGNOSTICS='1')
  p=subprocess.Popen([str(game/'Game.exe'),'-harness','-harness-active','-windowed','-cfg','bench.cfg'],cwd=game,env=env,startupinfo=startup,stdout=(root/(role+'-output.log')).open('w'),stderr=subprocess.STDOUT)
  processes.append(p);(root/(role+'.pid')).write_text(str(p.pid))
  print(role,p.pid,flush=True)
 wait(lambda: all((root/(r+'-user-data/startup.log')).exists() and 'main-menu-presented' in (root/(r+'-user-data/startup.log')).read_text() for r in ('host','client')))
 for role in ('host','client'):
  cmd(role,'netmenu')
 for key,value in [('net_squad_size',str(count)),('net_supply_level','14'),('net_starting_level','5'),('net_port',str(args.port))]:cmd('host','uiset '+key+' '+value)
 cmd('client','uiset net_port '+str(args.port));cmd('host','uiclick net_create');cmd('client','uiclick net_connect')
 wait(lambda: all(active(role) for role in ('host','client')),180)
 # Capture the queued-update catch-up immediately, before warming the scene.
 for role in ('host','client'):cmd(role,'perf arrival 200')
 wait(lambda:all((root/role/'_perf_arrival.csv').exists() and len((root/role/'_perf_arrival.csv').read_text().splitlines())>=201 for role in ('host','client')),120)
 for role in ('host','client'):
  print(role,active(role),flush=True);cmd(role,'screenshot')
 time.sleep(5)
 for role in ('host','client'):cmd(role,'perf idle '+str(args.idle_frames))
 wait(lambda: all((root/role/'_perf_idle.csv').exists() and len((root/role/'_perf_idle.csv').read_text().splitlines())>=args.idle_frames+1 for role in ('host','client')),240)
 for role in ('host','client'):
  cmd(role,'perf interaction '+str(args.frames))
 for cycle in range(args.cycles):
  for role in ('host','client'):
   cmd(role,'uipoint 450 145');time.sleep(.5)
   cmd(role,'uipoint 1230 700');time.sleep(.25)
   cmd(role,'uipoint 130 145');time.sleep(.25)
   cmd(role,'uipoint 130 145')
   cmd(role,'netmove 0 '+('2 0' if cycle%2==0 else '-2 0'))
  time.sleep(1)
 wait(lambda: all((root/role/'_perf_interaction.csv').exists() and len((root/role/'_perf_interaction.csv').read_text().splitlines())>=args.frames+1 for role in ('host','client')),240)
finally:
 for role,p in zip(('host','client'),processes):
  if p.poll() is None:
   try:cmd(role,'quit');p.wait(timeout=60)
   except Exception as e:raise RuntimeError('Test game did not exit gracefully: '+role+' PID '+str(p.pid)) from e
summary={}
for role in ('host','client'):
 for path in (root/role).glob('_perf_*.csv'):
  rows=list(csv.DictReader(path.open()))
  if not rows:continue
  values=sorted(float(r['frame_interval_ms']) for r in rows if float(r['frame_interval_ms'])>0)
  metrics={k:round(statistics.mean(float(r[k]) for r in rows),3) for k in ('frame_ms','interface_ms','world_ms','scene_ms','present_ms','gpu_ms')}
  metrics.update(samples=len(values),fps=round(1000/statistics.mean(values),2),median=round(statistics.median(values),3),p95=round(values[int((len(values)-1)*.95)],3),p99=round(values[int((len(values)-1)*.99)],3),max=round(max(values),3),over100=sum(v>100 for v in values),over500=sum(v>500 for v in values),over1000=sum(v>1000 for v in values))
  summary[role+'/'+path.stem]=metrics
(root/'summary.json').write_text(json.dumps(summary,indent=2))
print(json.dumps(summary,indent=2),flush=True)

failures=[name for name,metrics in summary.items() if (args.max_p99_ms>0 and metrics['p99']>args.max_p99_ms) or (args.max_frame_ms>0 and metrics['max']>args.max_frame_ms)]
if failures:
 print('Frame-time budget exceeded: '+', '.join(failures),file=sys.stderr)
 sys.exit(1)
