"""G10: compact listing of the sound-event CSV.

Usage: python3 -I G10_events.py <X2b_sound_events.csv> [filter]
Prints one line per (source, event) pair: group, type, event, in_data,
blocks, fired_in_FULL, n_samples, blueprint (shortened) and trigger.
"""
import csv
import sys


def main():
    path = sys.argv[1]
    flt = sys.argv[2] if len(sys.argv) > 2 else None
    with open(path, newline='', encoding='utf-8') as fh:
        rows = list(csv.DictReader(fh))
    for r in rows:
        if flt == 'live' and not (r['in_data'] == 'yes' and r['fired_in_FULL'] == 'yes'):
            continue
        if flt == 'silent' and not (r['in_data'] != 'yes' and r['fired_in_FULL'] == 'yes'):
            continue
        if flt == 'deadbound' and not (r['in_data'] == 'yes' and r['fired_in_FULL'] != 'yes'):
            continue
        bp = r['blueprint'].replace('\n', ' | ')
        trig = r['gameplay_trigger'].replace('\n', ' ')
        print('\t'.join([r['group'], r['object_type'], r['event'], r['in_data'],
                         r['blocks'], r['fired_in_FULL'], r['n_samples'],
                         r['type_reach'][:40], bp, trig[:220]]))
    print('rows', len(rows), file=sys.stderr)


if __name__ == '__main__':
    main()
