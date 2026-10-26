#!/usr/bin/env python3
"""Exercise tagged release policies with real temporary Git history; no network."""
import argparse
import contextlib
import importlib.util
import io
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('release_helper', ROOT / '.ci/package-release.py')
release = importlib.util.module_from_spec(spec)
spec.loader.exec_module(release)
DESKTOP = ('linux-x86_64', 'linux-arm64', 'windows-x86_64', 'macos-arm64', 'macos-x86_64')


class ReleaseTargets(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='rpcs3-release-policy-')
        self.cwd = os.getcwd()
        os.chdir(self.temp.name)
        self.git('init', '-q', '--initial-branch=main')
        self.git('config', 'user.name', 'Release Policy Fixture')
        self.git('config', 'user.email', 'fixture@example.invalid')
        version = Path('rpcs3/libretro/upstream.version')
        version.parent.mkdir(parents=True)
        version.write_text('RPCS3_VERSION=v0.0.43-20400\n')
        Path('CHANGELOG.md').write_text('## v0.1.0.2\n\nLegacy fixture.\n')
        self.commit('Legacy release')
        self.legacy = self.git('rev-parse', 'HEAD')
        self.git('tag', 'v0.1.0.2')
        manifest = Path('.ci/release-targets.json')
        manifest.parent.mkdir()
        manifest.write_text(json.dumps({'format_version': 1, 'targets': list(DESKTOP)}))
        self.commit('Desktop release policy')
        self.desktop = self.git('rev-parse', 'HEAD')
        self.git('tag', 'v0.1.0.3')
        self.git('update-ref', 'refs/remotes/origin/main', self.desktop)

    def tearDown(self):
        os.chdir(self.cwd)
        self.temp.cleanup()

    def git(self, *args):
        return subprocess.check_output(['git', *args], text=True, stderr=subprocess.PIPE).strip()

    def commit(self, message):
        self.git('add', '.')
        self.git('commit', '-qm', message)

    def plan(self, tag, platforms='all', reuse_run=''):
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            release.plan(argparse.Namespace(tag=tag, platforms=platforms, reuse_run=reuse_run))
        return json.loads(buf.getvalue())

    def test_old_immutable_tag_keeps_android(self):
        self.assertEqual(release.targets_for(self.legacy), release.TARGETS)
        plan = self.plan('v0.1.0.2')
        self.assertEqual(plan['source_commit'], self.legacy)
        self.assertEqual(plan['platforms'], 'linux,windows,macos,android')

    def test_future_tag_excludes_android(self):
        self.assertEqual(release.targets_for(self.desktop), DESKTOP)
        self.assertEqual(self.plan('v0.1.0.3')['platforms'], 'linux,windows,macos')

    def test_android_recovery_rejected_for_future_tag(self):
        with self.assertRaisesRegex(AssertionError, 'excluded'):
            self.plan('v0.1.0.3', 'android', '123')

    def test_partial_build_still_needs_prior_run(self):
        with self.assertRaisesRegex(AssertionError, 'previous release run'):
            self.plan('v0.1.0.3', 'windows')

    def test_manifest_comes_from_source_not_worktree(self):
        Path('.ci/release-targets.json').write_text('{}')
        self.assertEqual(release.targets_for(self.desktop), DESKTOP)
        self.assertEqual(release.targets_for(self.legacy), release.TARGETS)

    def assemble_fixture(self, tag, targets, output):
        incoming = Path(output + '-incoming')
        incoming.mkdir()
        for target in targets:
            (incoming / f'rpcs3-libretro-{tag}-{target}.zip').write_bytes(b'policy-only fixture')
        args = argparse.Namespace(tag=tag, directory=incoming, output=Path(output), notes=Path(output + '-notes.md'))
        # Binary/checksum verification is covered by the existing package
        # verifier. This fixture isolates manifest-driven package-set gating.
        with mock.patch.object(release, 'verify_archive',
                               side_effect=lambda path, tag, commit, target: {'target': target, 'source_commit': commit}):
            release.assemble(args)
        return args

    def test_future_assembly_requires_five_packages(self):
        args = self.assemble_fixture('v0.1.0.3', DESKTOP, 'desktop-package')
        info = json.loads((args.output / 'BUILD_INFO.json').read_text())
        self.assertEqual(len(info['packages']), 5)
        self.assertIn('All 5 packages', args.notes.read_text())
        self.assertIn('Android publication is paused', args.notes.read_text())
        with self.assertRaises(AssertionError):
            self.assemble_fixture('v0.1.0.3', DESKTOP[:-1], 'missing-package')
        with self.assertRaises(AssertionError):
            self.assemble_fixture('v0.1.0.3', DESKTOP + ('android-arm64',), 'extra-package')

    def test_legacy_assembly_still_requires_seven(self):
        args = self.assemble_fixture('v0.1.0.2', release.TARGETS, 'legacy-package')
        self.assertEqual(len(json.loads((args.output / 'BUILD_INFO.json').read_text())['packages']), 7)
        self.assertIn('All 7 packages', args.notes.read_text())

    def test_future_android_package_rejected_before_payload(self):
        with self.assertRaisesRegex(AssertionError, 'excluded'):
            release.package(argparse.Namespace(tag='v0.1.0.3', target='android-arm64'))

    def test_malformed_source_manifest_rejected(self):
        for targets in ([], ['unknown-arm64'], ['linux-arm64', 'linux-arm64'], ['linux-arm64']):
            Path('.ci/release-targets.json').write_text(json.dumps({'format_version': 1, 'targets': targets}))
            self.commit('Invalid policy fixture')
            with self.assertRaises(AssertionError):
                release.targets_for(self.git('rev-parse', 'HEAD'))


if __name__ == '__main__':
    unittest.main()
