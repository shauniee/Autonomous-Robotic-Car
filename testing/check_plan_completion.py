#!/usr/bin/env python3
"""Audit recorded plan completeness; this never executes or certifies a robot."""
import hashlib
import json
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
# Append new audits instead of rewriting historical evidence.
record = max((root / 'testing/integration/logs').glob('*completion.json'),
             default=None)
if record is None:
    raise SystemExit('Completion evidence record is missing; no completion claim.')
audit = json.loads(record.read_text())
manifest = json.loads((root / audit['source_manifest']).read_text())
changed = [name for name, digest in manifest['files'].items()
           if not (root / name).is_file()
           or hashlib.sha256((root / name).read_bytes()).hexdigest() != digest]
if changed:
    print('STALE EVIDENCE: source differs from the recorded software checks')
    for name in changed:
        print(name)
    raise SystemExit(2)
stages = audit['stages']
if {item['stage'] for item in stages} != set(range(10)):
    raise SystemExit('Every plan stage 0..9 must be audited.')
valid = {'PASS', 'PARTIAL', 'BLOCKED'}
for stage in stages:
    if stage['status'] not in valid:
        raise SystemExit('Invalid completion status.')
    if stage['status'] == 'PASS' and stage['requires_hardware']:
        evidence = stage.get('physical_evidence', [])
        if not evidence:
            raise SystemExit('Hardware stage cannot pass without physical evidence.')
        for name in evidence:
            physical = json.loads((root / name).read_text())
            for field in ('source_commit', 'image_sha256', 'operator',
                          'measurements', 'accepted_limits', 'raw_evidence'):
                if not physical.get(field):
                    raise SystemExit('Incomplete physical evidence metadata: ' + name)
    print(f"Stage {stage['stage']}: {stage['status']} — {stage['reason']}")
complete = all(item['status'] == 'PASS' for item in stages)
if audit['overall'] != ('COMPLETE' if complete else 'BLOCKED'):
    raise SystemExit('Overall status conflicts with the individual stages.')
print('Overall: ' + audit['overall'])
print('Software evidence is current. This audit does not execute hardware or set physical limits.')
sys.exit(0 if complete else 1)
