#!/usr/bin/env python3
"""Reject macOS libraries built for a newer OS than the release supports."""

import argparse
import json
from pathlib import Path
import re
import subprocess


def version(text):
    parts = tuple(int(item) for item in text.split('.'))
    return (parts + (0, 0, 0))[:3]


def deployment_versions(lines):
    command = ''
    for line in lines:
        text = line.strip()
        if text.startswith('cmd '):
            command = text.split()[-1]
        match = re.fullmatch(r'(minos|version)\s+([0-9.]+)', text)
        if match and ((command == 'LC_BUILD_VERSION' and match[1] == 'minos') or
                      (command == 'LC_VERSION_MIN_MACOSX' and match[1] == 'version')):
            yield version(match[2])


def inspect(path, maximum):
    with subprocess.Popen(['otool', '-l', str(path)], stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT, text=True) as process:
        versions = set(deployment_versions(process.stdout))
        assert process.wait() == 0, f'Cannot inspect {path}'
    assert versions, f'No macOS deployment target found in {path}'
    found = max(versions)
    assert found <= maximum, f'{path} requires macOS {found}, above {maximum}'
    return {'path': str(path), 'minimum_macos': '.'.join(map(str, found))}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--maximum', default='15.0')
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('libraries', nargs='+', type=Path)
    args = parser.parse_args()
    result = {'maximum_macos': args.maximum,
              'libraries': [inspect(path, version(args.maximum)) for path in args.libraries]}
    args.report.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
