#!/usr/bin/env python3
"""Check deployment guards with real object/archive fixtures, never load them."""

import importlib.util
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


class DeploymentTargets(unittest.TestCase):
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
