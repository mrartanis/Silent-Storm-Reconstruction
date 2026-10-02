"""Run native regressions and an optional, separate ordinary-input manifest."""
import argparse
import json
import pathlib
import re
import subprocess
import sys
import time
import xml.etree.ElementTree as ET

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build',type=pathlib.Path,required=True)
    parser.add_argument('--ctest',default='ctest')
    parser.add_argument('--configuration',default='RelWithDebInfo')
    parser.add_argument('--evidence',type=pathlib.Path,required=True)
    parser.add_argument('--graphics',type=pathlib.Path,help='JSON list of named scenario files')
    parser.add_argument('--run',type=pathlib.Path)
    parser.add_argument('--platform',choices=['windows','linux'])
    parser.add_argument('--target')
    parser.add_argument('--helper')
    parser.add_argument('--native-tests',nargs='+',help='Run exactly these required CTest cases for a focused regression')
    parser.add_argument('--map-reference',type=pathlib.Path,help='Matching --details root-map artifacts from another platform')
    args=parser.parse_args()
    if args.graphics and not all([args.run,args.platform,args.target]):
        parser.error('--graphics requires --run, --platform and --target')
    if (args.evidence/'report.json').exists():
        parser.error('Use a fresh evidence directory to preserve the previous report')
    args.evidence.mkdir(parents=True,exist_ok=True)
    report={'status':'running','native':[],'graphics':[],'started':time.time()}
    output=args.evidence/'report.json'
    def save(): output.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    save()
    native_xml=args.evidence/'native.xml'
    report['artifacts']={'native_log':str((args.evidence/'native.log').resolve()),
                         'junit':str(native_xml.resolve())}
    native_pattern=('^('+ '|'.join(re.escape(name) for name in args.native_tests) +')$'
                    if args.native_tests else '^(NativeCampaignSequence-.*|ChapterTravelTests|NativeScenarioRootMaps)$')
    cmd=[args.ctest,'--test-dir',str(args.build),'-C',args.configuration,'--output-on-failure',
         '--output-junit',str(native_xml),'-R',native_pattern]
    native_returncode=1
    with (args.evidence/'native.log').open('w',encoding='utf-8') as log:
        try:
            process=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,timeout=7500)
            native_returncode=process.returncode
        except (subprocess.TimeoutExpired,OSError) as error:
            report['native'].append({'status':'failed','error':str(error)})
    if native_xml.is_file():
        for case in ET.parse(native_xml).iter('testcase'):
            failure=case.find('failure'); skipped=case.find('skipped')
            report['native'].append({'name':case.get('name'),'seconds':case.get('time'),
                'status':'failed' if failure is not None else 'skipped' if skipped is not None else 'passed',
                'failure':(failure.text or failure.get('message') or 'See native.log and native.xml') if failure is not None else None})
    if not report['native']: report['native'].append({'status':'failed','error':'No native results produced'})
    required=(set(args.native_tests) if args.native_tests else
              {'NativeCampaignSequence-'+mode for mode in ('normal','scene-save','nested-save','intro-normal')} |
              {'ChapterTravelTests','NativeScenarioRootMaps'})
    missing=required-{row.get('name') for row in report['native']}
    if missing: report['native'].append({'status':'failed','error':'Required tests missing','tests':sorted(missing)})
    if args.map_reference:
        from CompareScenarioRootMaps import compare
        try:
            comparison=compare(args.map_reference,args.build/'scenario-root-maps')
            detail=args.evidence/'map-differences.json'
            detail.write_text(json.dumps(comparison,indent=2)+'\n',encoding='utf-8')
            report['native'].append({'name':'ScenarioRootMapDetails','status':comparison['status'],
                'report':str(detail.resolve()),'differences':comparison['differences']})
        except (OSError,ValueError) as error:
            report['native'].append({'name':'ScenarioRootMapDetails','status':'failed','error':str(error)})
    save()
    if args.graphics:
        for scenario in json.loads(args.graphics.read_text(encoding='utf-8')):
            if scenario.get('status')=='not_run':
                report['graphics'].append({'name':scenario['id'],'status':'not_run','reason':scenario['reason']})
                save(); continue
            steps=(args.graphics.parent/scenario['steps']).resolve()
            scenario_report=args.evidence/(scenario['id']+'.json')
            entry={'name':scenario['id'],'status':'running','report':str(scenario_report)}
            report['graphics'].append(entry); save()
            cmd=[sys.executable,str(pathlib.Path(__file__).with_name('Stage6Scenario.py')),
                 str(args.run),str(steps),'--platform',args.platform,'--target',args.target,
                 '--report',str(scenario_report)]
            if args.helper: cmd+=['--helper',args.helper]
            with (args.evidence/(scenario['id']+'.log')).open('w',encoding='utf-8') as log:
                try:
                    result=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,timeout=scenario.get('timeout',1200))
                    entry['status']='passed' if result.returncode==0 else 'failed'
                except subprocess.TimeoutExpired:
                    entry.update(status='failed',error='scenario process timeout')
                except OSError as error:
                    entry.update(status='failed',error=str(error))
            save()
    else:
        report['graphics'].append({'status':'not_run','reason':'graphical set not selected'})
    statuses=[row['status'] for row in report['native']+report['graphics']]
    report['status']='failed' if native_returncode or 'failed' in statuses else 'partial' if any(status!='passed' for status in statuses) else 'passed'
    report['elapsed_seconds']=round(time.time()-report['started'],3); save()
    print(f"{report['status']}: {output.resolve()}")
    return 1 if report['status']=='failed' else 2 if report['status']=='partial' else 0

if __name__=='__main__': raise SystemExit(main())
