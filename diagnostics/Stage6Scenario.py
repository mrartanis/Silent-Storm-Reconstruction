"""Replay graphical checks; clicks/keys/drags go through ordinary OS input."""
import argparse, json, os, pathlib, re, shutil, subprocess, sys, time, traceback
from PIL import Image, ImageChops
sys.stdout.reconfigure(encoding='utf-8',errors='backslashreplace')

p=argparse.ArgumentParser()
p.add_argument('run',type=pathlib.Path)
p.add_argument('steps',type=pathlib.Path)
p.add_argument('--platform',choices=['windows','linux'],required=True)
p.add_argument('--target',required=True)
p.add_argument('--helper')
p.add_argument('--capture-timeout',type=float,default=60,help='Allow slow emulated GPU readback without altering gameplay')
p.add_argument('--report', type=pathlib.Path)
a=p.parse_args()
channel=a.run/'game/_harness_cmd.txt'
log=a.run/'game/_saveload.log'
evidence=a.report.parent if a.report else a.run/'evidence'
evidence.mkdir(parents=True, exist_ok=True)
report_path=a.report or evidence/(a.steps.stem+'-report.json')
snapshots={}
snapshot_log_offsets={}
active_deadline=None
captured_frames=[]

def command(value):
    deadline=time.monotonic()+300
    if active_deadline is not None: deadline=min(deadline,active_deadline)
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
    result=log.read_bytes()[offset:].decode('latin1')
    with (evidence/(a.steps.stem+'-observations.jsonl')).open('a',encoding='utf-8') as output:
        output.write(json.dumps({'command':value,'result':result})+'\n')
    return result

def input_action(action,*values):
    args=[sys.executable,str(pathlib.Path(__file__).with_name('Stage6Input.py')),str(a.run),
          '--platform',a.platform,'--target',a.target]
    if a.helper: args+=['--helper',a.helper]
    remaining=max(.1,active_deadline-time.monotonic()) if active_deadline is not None else 300
    subprocess.run(args+[action,*map(str,values)],check=True,timeout=remaining)

def control(suffix, action="click", at=(.5,.5), text=None):
    if text is None: text=command('displaytree')
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

def records(text, tag):
    return [dict(re.findall(r'(\w+)=([^\s]+)',row))
            for row in re.findall(r'\['+re.escape(tag)+r'\] ([^\r\n]*)',text)]

def unit(text, pers):
    candidates=records(text,'bug-unit')
    rows=([row for row in candidates if row.get(pers)=='1'] if isinstance(pers,str)
          else [row for row in candidates if int(row['pers'])==pers])
    if len(rows)!=1: raise AssertionError(('unique unit required',pers,rows))
    return rows[0]

def distance(left,right):
    return sum((float(x)-float(y))**2 for x,y in zip(left.split(','),right.split(',')))**.5

def inventory(text,item):
    totals={}
    for row in records(text,'bug-item'):
        if int(row['item'])==item:
            owner=int(row['pers'])
            totals[owner]=totals.get(owner,0)+int(row['quantity'])
    totals['store']=sum(int(row['quantity']) for row in records(text,'bug-store') if int(row['item'])==item)
    totals['ground']=sum(int(row['quantity']) for row in records(text,'bug-ground') if int(row['item'])==item)
    return totals

remembered = {}
def run_step(step):
    print('step',step,flush=True)
    if 'snapshot' in step:
        snapshots[step['snapshot']]=command(step.get('query','bugstate'))
        snapshot_log_offsets[step['snapshot']]=log.stat().st_size
    elif 'start_combat' in step:
        state=records(command('bugstate'),'bug-state')[0]
        assert state['available']=='1',state
        if state['realtime']=='1': input_action('key','0x1c')
        run_step({'wait_until':'available=1 .*realtime=0','query':'bugstate','timeout':30})
    elif 'verify_action' in step:
        options=step['verify_action']
        world_before=records(snapshots[options['before']],'bug-state')[0]
        assert world_before['realtime']=='0',('AP check requires turn-based mode',world_before)
        before=unit(snapshots[options['before']],options['pers'])
        after=unit(command('bugstate'),options['pers'])
        assert int(after['script_sequence'])==0 and int(after['free_ap'])==0,(before,after)
        action=options['action']
        if action=='pose': assert before['pose']!=after['pose'],('pose not changed',before,after)
        elif action=='move': assert distance(before['position'],after['position'])>.1,('unit did not move',before,after)
        elif action=='shoot': assert 0<=int(after['ammo'])<int(before['ammo']),('no shot',before,after)
        else: raise ValueError(action)
        assert int(after['ap'])<int(before['ap']),('AP not spent on completed action',before,after)
        assert after['command']=='0',('action did not finish',after)
    elif 'assert_sequence_consistent' in step:
        result=command('bugstate')
        states=records(result,'bug-state')
        assert len(states)==1 and states[0]['available']=='1',result
        for row in records(result,'bug-unit'):
            assert row['script_sequence']==states[0]['sequence'],(states,row)
    elif 'assert_floor' in step:
        state=records(command('bugstate'),'bug-state')[0]
        assert state['available']=='1',state
        assert int(state['floor_min'])<=int(state['floor'])<=int(state['floor_max']),state
        ranges=records(command('bugstate'),'bug-floor')
        assert len(ranges)==1,ranges
        assert ranges[0]['camera_min']==ranges[0]['scene_min'] and ranges[0]['camera_max']==ranges[0]['scene_max'],ranges
        for key,value in step['assert_floor'].items():
            field={'min':'floor_min','max':'floor_max','current':'floor'}[key]
            assert int(state[field])==value,(field,value,state)
    elif 'assert_panel' in step:
        options=step['assert_panel']
        state=command('bugstate'); tree=command('displaytree')
        pers=int(unit(state,options['pers'])['pers'])
        model={int(row['skill']):row for row in records(state,'bug-skill') if int(row['pers'])==pers}
        bars={row['id']:float(row['value']) for row in records(tree,'display-progress') if 'id' in row}
        numbers={row['id']:float(row['value']) for row in records(tree,'display-number') if 'id' in row}
        images={row['id']:int(row['texture']) for row in records(tree,'display-image') if 'id' in row}
        for bar,skill in options['bars'].items():
            assert bar in bars,('missing progress bar',bar,tree)
            expected=float(model[skill]['progress'])
            assert abs(bars[bar]-expected)<.00001,(bar,expected,bars[bar])
        for label,skill in options.get('values',{}).items():
            assert numbers.get(label)==int(model[skill]['value']),(label,numbers.get(label),model[skill])
        for image,texture in options.get('images',{}).items():
            assert images.get(image)==texture,(image,texture,images.get(image))
        for skill,value in options.get('progress',{}).items():
            assert abs(float(model[int(skill)]['progress'])-value)<.00001,(skill,model[int(skill)],value)
        for pattern in options.get('arrows',[]): assert re.search(pattern,tree),('arrow state',pattern,tree)
    elif 'compare_panel' in step:
        options=step['compare_panel']
        reference=pathlib.Path(options['reference'])
        assert reference.is_file(),('approved reference required',str(reference))
        with Image.open(evidence/(options['capture']+'.png')) as actual, Image.open(reference) as expected:
            assert actual.size==expected.size,('resolution differs',actual.size,expected.size)
            region=options['region']
            left=actual.convert('RGB').crop(region); right=expected.convert('RGB').crop(region)
            difference=ImageChops.difference(left,right)
            pixels=list(difference.getdata())
            changed=sum(max(pixel)>options.get('channel_tolerance',0) for pixel in pixels)/len(pixels)
            difference.save(evidence/(options['capture']+'-panel-diff.png'))
            assert changed<=options.get('changed_fraction',0),(changed,options)
    elif 'inventory_transfer' in step:
        options=step['inventory_transfer']
        before_text=snapshots[options['before']]
        after_text=command('bugstate')
        before=inventory(before_text,options['item']); after=inventory(after_text,options['item'])
        recipient=int(unit(before_text,options['recipient'])['pers'])
        destination=options.get('destination','inventory')
        if destination not in ('inventory','backpack','ground'): raise ValueError(destination)
        leader=options['leader']
        if options.get('different_from_leader',True):
            assert recipient!=int(unit(before_text,leader)['pers']),('recipient must differ from shop interlocutor',recipient)
        if options.get('no_intermediate_leader'):
            transactions=records(log.read_bytes()[snapshot_log_offsets[options['before']]:].decode('latin1'),'inventory-move')
            transactions=[row for row in transactions if int(row['item'])==options['item']]
            assert transactions,('no executed inventory transfers observed',options)
            assert len({row['identity'] for row in transactions})==1,('more than one item transferred',transactions)
            for row in transactions:
                assert int(row['executor'])==recipient,('wrong transfer executor',recipient,row)
                assert int(row['source_unit']) in (-1,recipient) and int(row['target_unit']) in (-1,recipient),('intermediate inventory owner',recipient,row)
            assert transactions[0]['source_place']=='store',('transfer did not start in store',transactions)
            # Use names emitted from SItem's enum; UI CSlotInfo has a different enum.
            targets={'inventory':('slot','backpack','unit_anyplace'),'backpack':('backpack',),'ground':('ground',)}[destination]
            assert transactions[-1]['target_place'] in targets and int(transactions[-1]['target_unit'])==recipient,('wrong final placement',destination,transactions)
            if destination=='ground':
                landed=[row for row in records(after_text,'bug-ground')
                        if row['identity'].lower()==transactions[-1]['identity'].lower()]
                assert len(landed)==1,('transferred object absent or duplicated on ground',transactions,landed)
            print('inventory transfer trace',transactions,flush=True)
        assert sum(before.values())==sum(after.values()),('lost or duplicated item',before,after)
        for owner in set(before)|set(after):
            target_owner='ground' if destination=='ground' else recipient
            expected=options['quantity'] if owner==target_owner else -options['quantity'] if owner=='store' else 0
            assert after.get(owner,0)-before.get(owner,0)==expected,('wrong recipient/quantity',owner,expected,before,after)
        assert distance(unit(before_text,leader)['position'],unit(after_text,leader)['position'])<.01,('leader moved',leader)
        leader_id=unit(before_text,leader)['pers']
        if recipient!=int(leader_id):
            assert unit(before_text,leader)['ap']==unit(after_text,leader)['ap'],('leader spent AP',leader)
            def leader_inventory(text):
                return sorted(tuple(sorted(row.items())) for row in records(text,'bug-item') if row['pers']==leader_id)
            assert leader_inventory(before_text)==leader_inventory(after_text),('leader inventory changed',leader)
        assert not any(int(row['item'])==options['item'] and row['place']=='hand'
                       for row in records(after_text,'bug-item')),('transfer left item on cursor',options)
    elif 'command' in step: print(command(step['command']),flush=True)
    elif 'click_world' in step:
        options=step['click_world']
        position=list(map(float,unit(command('bugstate'),options['pers'])['position'].split(',')))
        target=[x+y for x,y in zip(position,options['offset'])]
        projection=records(command('projectpoint '+' '.join(map(str,target))),'bug-project')
        assert len(projection)==1 and projection[0].get('visible')=='1',('world point not visible',target,projection)
        input_action('click',*map(float,projection[0]['pixel'].split(',')))
    elif 'slot_item' in step:
        options=step['slot_item']
        items=records(command('displaytree'),'display-slot-item')
        candidates=[row for row in items if row.get('id')==options['slot'] and int(row['item'])==options['item']]
        assert candidates,('item absent from visible slot',options,items)
        input_action(options.get('action','click'),*map(float,candidates[0]['pixel'].split(',')))
    elif 'shoot_target' in step:
        # Prefer an actual visible/heard target icon. A safe entry area may
        # have none; then use a nearby projected ground point.
        candidates=[]; rectangle=None
        textures={463,*range(446,454),*range(666,675)}
        for line in command('displaytree').splitlines():
            if line.startswith('[display-control]'):
                match=re.search(r'clip=(-?\d+),(-?\d+),(-?\d+),(-?\d+)',line)
                rectangle=tuple(map(int,match.groups())) if match else None
            elif line.startswith('[display-image]') and rectangle:
                texture=int(re.search(r'texture=(\d+)',line).group(1))
                if texture in textures: candidates.append(rectangle)
        if candidates:
            x1,y1,x2,y2=candidates[0]
            input_action('move',(x1+x2)/2,(y1+y2)/2)
            print(command('bugstate'),flush=True)
            input_action('click',(x1+x2)/2,(y1+y2)/2)
        else:
            run_step({'click_world':step['shoot_target']})
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
        return chosen
    elif 'travel_to_sector' in step:
        # Campaign travel can enter a random encounter before the chosen zone.
        # Exiting that encounter is fixture preparation; the tested zone entry
        # and scene still use the ordinary marker click and real UI transition.
        options=step['travel_to_sector']
        chosen=run_step({'sector':options})
        destination={'type':chosen[1],'template':chosen[2],'visible':1}
        encounters=0
        while True:
            text=command('displaytree')
            match=re.search(r'\[chapter\] position=([-\d.]+),([-\d.]+) target=([-\d.]+),([-\d.]+)',text)
            if match:
                x,y,tx,ty=map(float,match.groups())
                if (x-tx)**2+(y-ty)**2<=1.1:
                    run_step({'sector':destination})
                    break
                time.sleep(.5)
                continue
            if re.search(r'id=(?:missionUI|dialogUI|movieUI|ingamemenu) ',text):
                encounters+=1
                assert encounters<=8,('too many intervening encounters',encounters)
                run_step({'advance_dialogue':True,'timeout':120})
                command('console @ExitToChapter()')
                run_step({'wait_until':'id=chaptermapUI ','timeout':120})
                run_step({'sector':destination})
                continue
            raise AssertionError(('unexpected travel screen',text))
    elif 'advance_dialogue' in step:
        deadline=time.monotonic()+step.get('timeout',180)
        saved_scene=False; skipped_scene=False
        while True:
            result=command('displaytree')
            if time.monotonic()>deadline: raise TimeoutError('dialogue')
            if re.search(r'id=ingamemenu ',result):
                control('ingamemenu/view/cancel',text=result)
                continue
            state=command('bugstate')
            worlds=records(state,'bug-state')
            available=len(worlds)==1 and worlds[0]['available']=='1'
            if available:
                for row in records(state,'bug-unit'):
                    assert row['script_sequence']==worlds[0]['sequence'],('scene transition flags',worlds,row)
                active=worlds[0]['sequence']=='1'
                if active and step.get('save_scene') and not saved_scene:
                    command('save '+step['save_scene']); command('load '+step['save_scene'])
                    saved_scene=True
                    continue
                if active and step.get('skip_scene') and not skipped_scene:
                    input_action('key','0x1'); skipped_scene=True
                    continue
                if re.search(r'id=missionUI ',result) and not active: break
            if re.search(r'id=movieUI ',result):
                time.sleep(.5)
                continue
            assert re.search(r'id=dialogUI ',result),('unexpected screen',result)
            # Use one observation for both choice and geometry. Dialogue buttons
            # may change while another harness query is being acknowledged.
            if re.search(r'depth=2 id=next ',result): control('dialogUI/next',text=result)
            elif re.search(r'depth=2 id=cancel ',result): control('dialogUI/cancel',text=result)
            else: time.sleep(.5)
            time.sleep(1)
        if step.get('save_scene'): assert saved_scene,'no active scene observed for save/load'
        if step.get('skip_scene'): assert skipped_scene,'no active scene observed for skip'
    elif 'dismiss_popups' in step:
        deadline=time.monotonic()+step.get('timeout',60)
        while re.search(r'id=ingamemenu ',command('displaytree')):
            if time.monotonic()>deadline: raise TimeoutError('popup dismissal')
            control('ingamemenu/view/cancel')
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
    elif 'leave_chapter' in step:
        if re.search(r'id=chaptermapUI ',command('displaytree')):
            run_step({'sector':{'type':2,'visible':1}})
            run_step({'wait_travel':True,'timeout':step.get('timeout',180)})
            run_step({'sector':{'type':2,'visible':1}})
    elif 'wait_until' in step or 'wait_not' in step:
        deadline=time.monotonic()+step.get('timeout',300)
        while True:
            result=command(step.get('query','displaytree'))
            matched=bool(re.search(step.get('wait_until',step.get('wait_not')),result))
            if matched == ('wait_until' in step): break
            if time.monotonic()>deadline: raise TimeoutError(step)
            time.sleep(.5)
    elif 'control' in step: control(step['control'],action=step.get('action','click'),at=step.get('at',(.5,.5)))
    elif 'if_control' in step:
        text=command('displaytree')
        if re.search(r'id='+re.escape(step['if_control'].split('/')[-1])+r' ',text):
            control(step['if_control'],text=text)
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
        captured_frames.append(str(output.resolve()))
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

result={'scenario':str(a.steps.resolve()),'status':'running','steps':[],
        'artifacts':{'observations':str((evidence/(a.steps.stem+'-observations.jsonl')).resolve()),
                     'game_log':str((evidence/(a.steps.stem+'-game.log')).resolve()),'frames':captured_frames}}
report_path.parent.mkdir(parents=True,exist_ok=True)
try:
    for index, step in enumerate(json.loads(a.steps.read_text(encoding='utf-8'))):
        entry={'index':index,'action':step,'status':'running'}
        result['steps'].append(entry)
        started=time.monotonic()
        active_deadline=started+step.get('timeout',300)
        try:
            run_step(step)
            entry['status']='passed'
        except Exception as error:
            entry.update(status='failed',error=str(error),traceback=traceback.format_exc())
            result['status']='failed'
            # A screenshot failure cannot turn the failed scenario into a pass.
            active_deadline=time.monotonic()+a.capture_timeout
            try: run_step({'capture':a.steps.stem+'-failure'})
            except Exception as capture_error: result['capture_error']=str(capture_error)
            raise
        finally:
            entry['elapsed_seconds']=round(time.monotonic()-started,3)
            report_path.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    result['status']='passed'
    print('Scenario complete',flush=True)
finally:
    if log.exists(): shutil.copy2(log,evidence/(a.steps.stem+'-game.log'))
    report_path.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
