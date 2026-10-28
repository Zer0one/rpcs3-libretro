#!/usr/bin/env python3
"""Fetch exact compatible Homebrew bottles and verify them before installation."""

import argparse
import hashlib
import json
from pathlib import Path
import re
import urllib.request


def select_bottle(metadata, tag):
    bottle = metadata['bottle']['stable']
    selected = bottle['files'].get(tag)
    if not selected:
        raise ValueError(f'{metadata["name"]}: no bottle for {tag}')
    digest = selected['sha256']
    if not re.fullmatch('[0-9a-f]{64}', digest):
        raise ValueError('Invalid bottle checksum')
    if not selected['url'].startswith('https://ghcr.io/v2/homebrew/core/') or not selected['url'].endswith('/blobs/sha256:' + digest):
        raise ValueError('Unexpected bottle registry or digest URL')
    revision = metadata.get('revision', 0)
    version = metadata['versions']['stable'] + (f'_{revision}' if revision else '')
    rebuild = bottle.get('rebuild', 0)
    filename = f'{metadata["name"]}--{version}.{tag}.bottle' + (f'.{rebuild}' if rebuild else '') + '.tar.gz'
    if Path(filename).name != filename:
        raise ValueError('Invalid bottle filename')
    return dict(name=metadata['name'], filename=filename, url=selected['url'], sha256=digest)


def fetch(formula, tag, output, opener=urllib.request.urlopen):
    with opener('https://formulae.brew.sh/api/formula/' + formula + '.json', timeout=60) as response:
        metadata = json.load(response)
    if metadata['name'] != formula:
        raise ValueError('Formula identity mismatch')
    selected = select_bottle(metadata, tag)
    destination = output / selected['filename']
    temporary = destination.with_suffix(destination.suffix + '.part')
    # Homebrew's own public, anonymous GHCR download credential (brew.sh).
    request = urllib.request.Request(selected['url'], headers={'Authorization': 'Bearer QQ=='})
    digest = hashlib.sha256()
    try:
        with opener(request, timeout=120) as response, temporary.open('wb') as target:
            while data := response.read(1024 * 1024):
                digest.update(data)
                target.write(data)
        if digest.hexdigest() != selected['sha256']:
            raise ValueError(f'{formula}: bottle checksum mismatch')
        temporary.replace(destination)
    finally:
        temporary.unlink(missing_ok=True)
    return dict(selected, path=str(destination.resolve()))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--tag', default='arm64_sequoia')
    parser.add_argument('formulas', nargs='+', choices=['zstd', 'molten-vk', 'llvm@22'])
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    items = [fetch(formula, args.tag, args.output) for formula in args.formulas]
    (args.output / 'manifest.json').write_text(json.dumps(items, indent=2) + '\n')
    (args.output / 'paths.txt').write_text(''.join(item['path'] + '\n' for item in items))
    print(json.dumps(items, indent=2))


if __name__ == '__main__':
    main()
