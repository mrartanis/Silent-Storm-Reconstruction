"""Compare --details artifacts without replacing the approved map digest."""
import argparse
import itertools
import json
import pathlib
import re

DETAIL = re.compile(r'^detail component=(\S+) index=(\d+) stream=(\S+) value=([0-9A-F]+) signed=(-?\d+)$', re.M)

def compare(reference, actual, expected_maps=52):
    ids = sorted(set(f.stem for f in reference.glob('[0-9]*.txt')) |
                 set(f.stem for f in actual.glob('[0-9]*.txt')), key=int)
    if not ids:
        raise ValueError('No map detail artifacts found')
    differences = []
    if len(ids)!=expected_maps:
        differences.append({'error':'incomplete root set','expected_maps':expected_maps,'actual_maps':len(ids)})
    for variant in ids:
        paths = [directory / (variant + '.txt') for directory in (reference, actual)]
        if not all(path.is_file() for path in paths):
            differences.append({'map': int(variant), 'error': 'missing artifact',
                                'reference': paths[0].is_file(), 'actual': paths[1].is_file()})
            continue
        texts = [path.read_text(errors='replace') for path in paths]
        details = [DETAIL.findall(text) for text in texts]
        if not all(details):
            raise ValueError(f'Map {variant}: --details output is required')
        for expected, observed in itertools.zip_longest(*details):
            if expected != observed:
                row = expected or observed
                differences.append({'map': int(variant), 'component': row[0],
                                    'index': int(row[1]), 'stream': row[2],
                                    'expected': expected, 'actual': observed})
                break
        else:
            # This also catches script-byte differences and incomplete output.
            summaries = [re.findall(r'^(?:built=|routes |behavior_digest=|component_digest |behavior_components ).*', text, re.M)
                         for text in texts]
            if summaries[0] != summaries[1]:
                differences.append({'map': int(variant), 'component': 'summary',
                                    'expected': summaries[0], 'actual': summaries[1]})
    return {'status': 'failed' if differences else 'passed', 'maps': len(ids),
            'reference': str(reference.resolve()), 'actual': str(actual.resolve()),
            'differences': differences}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('reference', type=pathlib.Path)
    parser.add_argument('actual', type=pathlib.Path)
    parser.add_argument('--report', type=pathlib.Path, required=True)
    parser.add_argument('--expected-maps',type=int,default=52)
    args = parser.parse_args()
    result = compare(args.reference, args.actual,args.expected_maps)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    for difference in result['differences']:
        print(json.dumps(difference))
    print(f"{result['status']}: {result['maps']} maps")
    return result['status'] != 'passed'

if __name__ == '__main__':
    raise SystemExit(main())
