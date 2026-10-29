#!/usr/bin/env python3
"""Check deployment guards with real object/archive fixtures, never load them."""

import importlib.util
import hashlib
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('deployment_check', ROOT / '.ci/check-macos-deployment-target.py')
check = importlib.util.module_from_spec(spec)
spec.loader.exec_module(check)
spec_bottles = importlib.util.spec_from_file_location('bottles', ROOT / '.ci/fetch-macos-release-bottles.py')
bottles = importlib.util.module_from_spec(spec_bottles)
spec_bottles.loader.exec_module(bottles)


class DeploymentTargets(unittest.TestCase):
    @staticmethod
    def bottle_metadata(payload):
        digest = hashlib.sha256(payload).hexdigest()
        return {'name': 'zstd', 'versions': {'stable': '1.5.7'}, 'revision': 1,
                'bottle': {'stable': {'rebuild': 0, 'files': {'arm64_sequoia': {
                    'url': 'https://ghcr.io/v2/homebrew/core/zstd/blobs/sha256:' + digest,
                    'sha256': digest}}}}}

    def test_explicit_bottle_selection(self):
        metadata = self.bottle_metadata(b'fixture')
        selected = bottles.select_bottle(metadata, 'arm64_sequoia')
        self.assertEqual(selected['filename'], 'zstd--1.5.7_1.arm64_sequoia.bottle.tar.gz')
        with self.assertRaisesRegex(ValueError, 'no bottle'):
            bottles.select_bottle(metadata, 'arm64_sonoma')
        metadata['bottle']['stable']['files']['arm64_sequoia']['url'] = 'https://example.invalid/bottle'
        with self.assertRaisesRegex(ValueError, 'registry'):
            bottles.select_bottle(metadata, 'arm64_sequoia')

    def test_download_is_verified_before_replacement(self):
        payload = b'verified synthetic bottle'
        metadata = self.bottle_metadata(payload)
        with tempfile.TemporaryDirectory(prefix='rpcs3-bottle-fetch-') as temporary:
            output = Path(temporary)
            responses = iter([json.dumps(metadata).encode(), payload])
            item = bottles.fetch('zstd', 'arm64_sequoia', output,
                                 opener=lambda *a, **kw: io.BytesIO(next(responses)))
            path = Path(item['path'])
            self.assertEqual(path.read_bytes(), payload)
            responses = iter([json.dumps(metadata).encode(), b'corrupt'])
            with self.assertRaisesRegex(ValueError, 'checksum mismatch'):
                bottles.fetch('zstd', 'arm64_sequoia', output,
                              opener=lambda *a, **kw: io.BytesIO(next(responses)))
            self.assertEqual(path.read_bytes(), payload)
            self.assertFalse(list(output.glob('*.part')))

    def test_legacy_and_modern_commands(self):
        lines = ['cmd LC_BUILD_VERSION', 'minos 15.0', 'sdk 26.0',
                 'cmd LC_ID_DYLIB', 'version 99.0',
                 'cmd LC_VERSION_MIN_MACOSX', 'version 10.15']
        self.assertEqual(list(check.deployment_versions(lines)), [(15, 0, 0), (10, 15, 0)])

    def test_dependency_install_refuses_local_host(self):
        with tempfile.TemporaryDirectory(prefix='rpcs3-brew-guard-') as temporary:
            root = Path(temporary)
            calls = root / 'unexpected-brew-call'
            brew = root / 'brew'
            brew.write_text('#!/bin/sh\necho unexpected > "$TASK_BREW_CALLS"\nexit 91\n')
            brew.chmod(0o755)
            env = dict(os.environ, GITHUB_ACTIONS='false', RUNNER_ENVIRONMENT='self-hosted',
                       PATH=str(root) + os.pathsep + os.environ['PATH'], TASK_BREW_CALLS=str(calls))
            result = subprocess.run(['/bin/bash', str(ROOT / '.ci/prepare-macos-release-deps.sh')], env=env,
                                    capture_output=True, text=True, timeout=10)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(result.stdout, '')
            self.assertIn('ephemeral GitHub-hosted', result.stderr)
            self.assertFalse(calls.exists())

    def test_incomplete_download_never_uninstalls_libraries(self):
        for download_status in [0, 1]:
            with self.subTest(download_status=download_status), tempfile.TemporaryDirectory(prefix='rpcs3-bottle-guard-') as temporary:
                root = Path(temporary)
                calls = root / 'brew-calls'
                for name, content in {
                    'uname': '#!/bin/sh\necho arm64\n',
                    'brew': '#!/bin/sh\necho "$*" >> "$TASK_BREW_CALLS"\nexit 0\n',
                    'python3': f'#!/bin/sh\nexit {download_status}\n',
                }.items():
                    path = root / name
                    path.write_text(content)
                    path.chmod(0o755)
                env = dict(os.environ, GITHUB_ACTIONS='true', RUNNER_ENVIRONMENT='github-hosted',
                           PATH=str(root) + os.pathsep + os.environ['PATH'], TASK_BREW_CALLS=str(calls))
                result = subprocess.run(['/bin/bash', str(ROOT / '.ci/prepare-macos-release-deps.sh')], env=env,
                                        cwd=root, capture_output=True, text=True, timeout=10)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(calls.read_text().splitlines(), ['install ninja vulkan-headers'])

    def test_local_bottle_mode_is_scoped_to_verified_install(self):
        with tempfile.TemporaryDirectory(prefix='rpcs3-bottle-mode-') as temporary:
            root = Path(temporary)
            calls = root / 'brew-calls'
            for name, content in {
                'uname': '#!/bin/sh\necho arm64\n',
                'brew': '#!/bin/sh\nprintf "developer=%s %s\\n" "${HOMEBREW_DEVELOPER:-}" "$*" >> "$TASK_BREW_CALLS"\nif [ "$1" = --prefix ]; then echo /fixture; fi\n',
                'python3': '#!/bin/sh\ncase "$1" in *fetch-macos-release-bottles.py) mkdir -p macos-sequoia-bottles; for name in zstd molten-vk llvm; do printf fixture > "macos-sequoia-bottles/$name.bottle.tar.gz"; done; for name in zstd molten-vk llvm; do printf "%s/macos-sequoia-bottles/%s.bottle.tar.gz\\n" "$PWD" "$name"; done > macos-sequoia-bottles/paths.txt;; esac\n',
            }.items():
                path = root / name
                path.write_text(content)
                path.chmod(0o755)
            env = {key: value for key, value in os.environ.items()
                   if key != 'HOMEBREW_DEVELOPER'}
            env.update(GITHUB_ACTIONS='true', RUNNER_ENVIRONMENT='github-hosted',
                       PATH=str(root) + os.pathsep + os.environ['PATH'], TASK_BREW_CALLS=str(calls),
                       GITHUB_ENV=str(root / 'github-env'))
            result = subprocess.run(['/bin/bash', str(ROOT / '.ci/prepare-macos-release-deps.sh')],
                                    env=env, cwd=root, capture_output=True, text=True, timeout=10)
            self.assertEqual(result.returncode, 0, result.stderr)
            records = calls.read_text().splitlines()
            installs = [line for line in records if '.bottle.tar.gz' in line]
            self.assertEqual(len(installs), 3)
            self.assertTrue(all(line.startswith('developer=1 install ') for line in installs))
            self.assertTrue(all(line.startswith('developer= ') for line in records if line not in installs))

    @unittest.skipUnless(sys.platform == 'darwin', 'Native Mach-O fixture')
    def test_archive_checks_every_member(self):
        with tempfile.TemporaryDirectory(prefix='rpcs3-macos-target-') as temporary:
            root = Path(temporary)
            source = root / 'fixture.c'
            source.write_text('int deployment_fixture(void) { return 0; }\n')
            for name, target in [('old', '15.0'), ('new', '26.0')]:
                subprocess.run(['clang', '-target', f'arm64-apple-macos{target}', '-c',
                                str(source), '-o', str(root / f'{name}.o')], check=True)
            subprocess.run(['ar', 'rcs', str(root / 'old.a'), str(root / 'old.o')], check=True)
            subprocess.run(['ar', 'rcs', str(root / 'mixed.a'), str(root / 'old.o'), str(root / 'new.o')], check=True)
            self.assertEqual(check.inspect(root / 'old.a', (15, 0, 0))['minimum_macos'], '15.0.0')
            with self.assertRaisesRegex(AssertionError, 'above'):
                check.inspect(root / 'mixed.a', (15, 0, 0))
            with self.assertRaisesRegex(AssertionError, 'above'):
                check.inspect(root / 'new.o', (15, 0, 0))


if __name__ == '__main__':
    unittest.main()
