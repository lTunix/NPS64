#!/usr/bin/env python3
"""Summarize Vulkan entry/return checkpoints, without inferring a crash cause."""
import argparse
import json
from pathlib import Path
import re


def analyze(text):
    # The application log appends sessions. Never mix an earlier crash with this run.
    sessions = list(re.finditer(r'^NPS64 Vulkan Game Test .+$', text, re.M))
    if sessions:
        text = text[sessions[-1].start():]
    pending = {}
    errors = []
    entries = returns = 0
    for line in text.splitlines():
        match = re.search(r'\[gpu-api\] id=(\d+) (begin|end) (vk\w+)(?: (.*))?$', line)
        if not match:
            continue
        number, phase, name, details = match.groups()
        if phase == 'begin':
            entries += 1
            if number in pending:
                errors.append('Duplicate entry id=' + number)
            pending[number] = {'id': int(number), 'api': name, 'details': details or ''}
        else:
            returns += 1
            call = pending.pop(number, None)
            if not call or call['api'] != name:
                errors.append('Unmatched return id=' + number + ' ' + name)
    run = re.findall(r'^\[run\] frame=(\d+) (begin|end).*$', text, re.M)
    checkpoints = [line for line in text.splitlines() if '[rdp-checkpoint]' in line]
    return {'warning': 'An entry without return narrows the interval; it does not identify CPU versus GPU fault.',
            'entries': entries, 'returns': returns,
            'pending_calls': sorted(pending.values(), key=lambda call: call['id']),
            'log_errors': errors,
            'last_core_checkpoint': run[-1] if run else None,
            'last_renderer_checkpoints': checkpoints[-6:]}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('log', type=Path)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    result = json.dumps(analyze(args.log.read_text(errors='replace')), indent=2) + '\n'
    if args.output:
        args.output.write_text(result)
    print(result, end='')
