#!/usr/bin/env python3
"""Build and verify canonical RPCS3 Libretro release packages without loading a core."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import zipfile


TARGETS = (
    "linux-x86_64", "linux-arm64", "windows-x86_64",
    "macos-arm64", "macos-x86_64", "android-arm64", "android-x86_64",
)
EXPORTS = ("retro_init", "retro_run", "retro_get_system_info", "retro_load_game")


def git(*args: str) -> str:
    return subprocess.check_output(["git", *args], text=True).strip()


def digest(path: Path) -> str:
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def upstream(commit: str) -> str:
    text = git("show", f"{commit}:rpcs3/libretro/upstream.version")
    return re.search(r"^RPCS3_VERSION=(v\d+\.\d+\.\d+-\d+)$", text, re.M)[1]


def check_tag(tag: str) -> None:
    if not re.fullmatch(r"v\d+\.\d+\.\d+\.\d+", tag):
        raise ValueError("Release tags must use vMAJOR.MINOR.PATCH.BUILD")


def plan(args: argparse.Namespace) -> None:
    assert args.platforms in ("all", "linux", "windows", "macos", "android")
    if args.platforms != "all":
        assert args.tag and args.reuse_run, "Partial release builds require a tag and a previous release run"
    if args.reuse_run:
        assert args.tag and re.fullmatch(r"[1-9][0-9]*", args.reuse_run)
        previous = json.loads(args.reuse_metadata.read_text())
        assert str(previous["id"]) == args.reuse_run
        assert previous["status"] == "completed"
        assert previous["path"].split("@", 1)[0] == ".github/workflows/release.yml"
        assert previous["event"] in ("push", "workflow_dispatch")
        assert previous["head_repository"]["full_name"] == os.environ["GITHUB_REPOSITORY"]
        subprocess.run(["git", "merge-base", "--is-ancestor", previous["head_sha"], "origin/main"], check=True)
    if args.tag:
        check_tag(args.tag)
        commit = git("rev-parse", "--verify", f"refs/tags/{args.tag}^{{commit}}")
        subprocess.run(["git", "merge-base", "--is-ancestor", commit, "origin/main"], check=True)
    else:
        commit = git("rev-parse", "HEAD")
    result = {"source_commit": commit, "release_tag": args.tag, "upstream": upstream(commit),
              "platforms": args.platforms, "reuse_run_id": args.reuse_run}
    if os.environ.get("GITHUB_OUTPUT"):
        with open(os.environ["GITHUB_OUTPUT"], "a", encoding="utf-8") as stream:
            for key, value in result.items():
                stream.write(f"{key}={value}\n")
    print(json.dumps(result, indent=2))


def inspect_binary(data: bytes, target: str) -> dict:
    system, architecture = target.split("-", 1)
    if system in ("linux", "android"):
        assert data[:6] == b"\x7fELF\x02\x01", "Expected little-endian ELF64"
        assert struct.unpack_from("<H", data, 16)[0] == 3, "Expected shared object"
        assert struct.unpack_from("<H", data, 18)[0] == (183 if architecture == "arm64" else 62)
        return {"format": "ELF64"}
    if system == "windows":
        assert data[:2] == b"MZ", "Expected PE DLL"
        offset = struct.unpack_from("<I", data, 0x3C)[0]
        assert data[offset:offset + 4] == b"PE\0\0"
        assert struct.unpack_from("<H", data, offset + 4)[0] == 0x8664
        assert struct.unpack_from("<H", data, offset + 22)[0] & 0x2000, "Expected DLL flag"
        return {"format": "PE32+"}
    assert data[:4] == b"\xcf\xfa\xed\xfe", "Expected a thin little-endian Mach-O64"
    assert struct.unpack_from("<I", data, 4)[0] == (0x100000C if architecture == "arm64" else 0x1000007)
    commands = struct.unpack_from("<I", data, 16)[0]
    position = 32
    minimum = None
    for _ in range(commands):
        command, size = struct.unpack_from("<II", data, position)
        assert size >= 8 and position + size <= len(data)
        if command == 0x32:
            minimum = struct.unpack_from("<I", data, position + 12)[0]
        elif command == 0x24:
            minimum = struct.unpack_from("<I", data, position + 8)[0]
        position += size
    assert minimum is not None, "Missing macOS deployment target"
    return {"format": "Mach-O64", "minimum_macos": f"{minimum >> 16}.{minimum >> 8 & 255}.{minimum & 255}"}


def canonical_binary(target: str) -> str:
    return "rpcs3_libretro." + ("dll" if target.startswith("windows") else "dylib" if target.startswith("macos") else "so")


def package(args: argparse.Namespace) -> None:
    target = args.target.replace("aarch64", "arm64")
    assert target in TARGETS, target
    if args.tag:
        check_tag(args.tag)
    commit = git("rev-parse", "HEAD")
    if args.tag:
        assert git("rev-parse", f"refs/tags/{args.tag}^{{commit}}") == commit
    subprocess.run(["git", "diff", "--quiet"], check=True)
    short = git("rev-parse", "--short=7", "HEAD")
    version = upstream(commit)
    canonical = canonical_binary(target)
    suffix = Path(canonical).suffix
    inputs = sorted(args.directory.glob(f"rpcs3_libretro*{suffix}"))
    assert len(inputs) == 1, f"Expected one core in {args.directory}: {inputs}"
    data = inputs[0].read_bytes()
    binary_info = inspect_binary(data, target)
    assert all(name.encode() in data for name in EXPORTS), "Missing Libretro entry-point names"
    embedded = f"{version} (libretro core {short})"
    assert embedded.encode() in data, f"Wrong embedded source identity: {embedded}"
    if target.startswith("macos"):
        assert b"rpcs3_moltenvk_queue_submission" in data
        assert b"@rpath/MoltenVK\0" in data, "Expected the frontend MoltenVK provider"
    info = args.directory / "rpcs3_libretro.info"
    assert info.is_file() and 'corename = "RPCS3"' in info.read_text()
    args.output.mkdir(parents=True, exist_ok=True)
    label = args.tag or f"verification-{short}"
    stage = args.output / target
    stage.mkdir(exist_ok=False)
    shutil.copy2(inputs[0], stage / canonical)
    shutil.copy2(info, stage / info.name)
    for dependency in sorted(args.directory.glob(f"*{suffix}")):
        if dependency == inputs[0]:
            continue
        assert target.startswith("macos") and dependency.name.startswith("libzstd_macos_"), dependency
        shutil.copy2(dependency, stage / dependency.name)
    shutil.copy2("LICENSE", stage / "LICENSE")
    (stage / "SOURCE_COMMIT.txt").write_text(commit + "\n")
    (stage / "README.md").write_text(
        f"# RPCS3 Libretro {label}\n\nTarget: `{target}`. Upstream: `{version}`.\n\n"
        "Copy the canonical core and any adjacent shared-library dependency into RetroArch's cores directory. "
        "Copy rpcs3_libretro.info into its info directory. Firmware and games are not included.\n\n"
        "On macOS use the frontend's MoltenVK framework; this package does not ship or replace a Vulkan driver. "
        "MoltenVK Queue Submission defaults to Sync. Async remains experimental; save the option and restart "
        "RetroArch to apply it. The final-driver 3D/pause-resume and texture-anomaly checks remain open.\n\n"
        "These packages pass build, binary identity and checksum checks. Remote compilation is not a gameplay, "
        "audio, controller or physical-hardware test. See BUILD_INFO.json for the source and workflow run.\n"
    )
    modules = {}
    for line in git("ls-tree", "-r", "HEAD").splitlines():
        if line.startswith("160000 "):
            entry, path = line.split("\t", 1)
            modules[path] = entry.split()[2]
    metadata = {
        "release_tag": args.tag or None, "source_commit": commit,
        "upstream_version": version, "embedded_core_version": embedded,
        "target": target, "binary": canonical, **binary_info,
        "workflow_run": os.environ.get("GITHUB_RUN_ID"),
        "workflow_commit": os.environ.get("GITHUB_SHA"),
        "submodules": modules, "gpu_runtime_test": False,
        "checks": ["binary architecture", "Libretro entry-point names", "embedded source commit", "payload checksums"],
    }
    (stage / "BUILD_INFO.json").write_text(json.dumps(metadata, indent=2) + "\n")
    (stage / "SHA256SUMS").write_text("".join(f"{digest(path)}  {path.name}\n" for path in sorted(stage.iterdir()) if path.is_file()))
    archive = args.output / f"rpcs3-libretro-{label}-{target}.zip"
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as stream:
        for path in sorted(stage.iterdir()):
            stream.write(path, path.name)
    verify_archive(archive, args.tag, commit, target)
    print(archive)


def verify_archive(path: Path, tag: str, commit: str, target: str) -> dict:
    with zipfile.ZipFile(path) as archive:
        assert archive.testzip() is None
        names = archive.namelist()
        assert len(names) == len(set(names)) and all(Path(name).name == name for name in names)
        required = {canonical_binary(target), "rpcs3_libretro.info", "README.md", "LICENSE", "SOURCE_COMMIT.txt", "BUILD_INFO.json", "SHA256SUMS"}
        assert required <= set(names), f"Missing package files: {required - set(names)}"
        extra = set(names) - required
        assert all(target.startswith("macos") and re.fullmatch(r"libzstd_macos_(arm64|x86_64)\.dylib", name) for name in extra), extra
        checksums = {}
        for line in archive.read("SHA256SUMS").decode().splitlines():
            checksum, name = line.split("  ", 1)
            assert name not in checksums
            checksums[name] = checksum
        assert set(checksums) == set(names) - {"SHA256SUMS"}
        for name, checksum in checksums.items():
            assert hashlib.sha256(archive.read(name)).hexdigest() == checksum, name
        metadata = json.loads(archive.read("BUILD_INFO.json"))
        assert metadata["source_commit"] == commit
        assert metadata["release_tag"] == (tag or None) and metadata["target"] == target
        assert archive.read("SOURCE_COMMIT.txt").decode().strip() == commit
        inspect_binary(archive.read(canonical_binary(target)), target)
        return metadata


def assemble(args: argparse.Namespace) -> None:
    check_tag(args.tag)
    commit = git("rev-parse", f"refs/tags/{args.tag}^{{commit}}")
    archives = list(args.directory.rglob("*.zip"))
    expected = {f"rpcs3-libretro-{args.tag}-{target}.zip": target for target in TARGETS}
    assert len(archives) == len(expected) and {path.name for path in archives} == set(expected)
    args.output.mkdir(parents=True, exist_ok=False)
    metadata = []
    for path in sorted(archives):
        item = verify_archive(path, args.tag, commit, expected[path.name])
        item.update(archive=path.name, archive_sha256=digest(path))
        metadata.append(item)
        shutil.copy2(path, args.output / path.name)
    (args.output / "BUILD_INFO.json").write_text(json.dumps({"release_tag": args.tag, "source_commit": commit, "packages": metadata}, indent=2) + "\n")
    (args.output / "SHA256SUMS").write_text("".join(f"{digest(path)}  {path.name}\n" for path in sorted(args.output.iterdir()) if path.is_file()))
    changelog = git("show", f"{commit}:CHANGELOG.md")
    heading = re.search(rf"^## {re.escape(args.tag)}\b.*$", changelog, re.M)
    changes = changelog[heading.end():].split("\n## ", 1)[0].strip() if heading else "See CHANGELOG.md at the tagged source revision."
    rows = "\n".join(f"| `{item['target']}` | {item.get('minimum_macos', 'See target system requirements')} |" for item in metadata)
    args.notes.write_text(
        f"RPCS3 Libretro **{args.tag}**, based on **{upstream(commit)}**. "
        f"Source commit: `{commit}`. All seven packages were built by GitHub Actions from this exact tag.\n\n"
        "### Changes\n\n" + changes + "\n\n### Packages\n\n| Target | Minimum macOS |\n| --- | --- |\n" + rows + "\n\n"
        "Each ZIP includes the canonical core, matching .info, any packaged runtime dependency, "
        "source/workflow metadata and payload checksums. Verify downloads with SHA256SUMS. "
        "MoltenVK, firmware and games are not included. BUILD_INFO.json records all seven source identities.\n\n"
        "### Validation And Limits\n\n"
        "Publication requires all seven remote builds and package checks to succeed. Compilation and static "
        "artifact checks do not establish frontend load or gameplay on the target systems. Android remains "
        "experimental. Async is experimental and defaults to Sync on macOS; save the option and restart RetroArch "
        "to apply it. The final-driver RPCS3 3D/pause-resume acceptance and prior texture anomaly remain unresolved.\n"
    )
    print(json.dumps({"tag": args.tag, "commit": commit, "packages": len(metadata)}, indent=2))


def verify_upload(args: argparse.Namespace) -> None:
    assets = json.loads(args.metadata.read_text())["assets"]
    expected = {path.name: digest(path) for path in args.directory.iterdir() if path.is_file()}
    assert {item["name"] for item in assets} == set(expected), "Incomplete release assets"
    for item in assets:
        assert item.get("digest") == "sha256:" + expected[item["name"]], item["name"]
    print(f"Verified {len(assets)} uploaded asset digests")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    prepare = subparsers.add_parser("plan")
    prepare.add_argument("--tag", default="")
    prepare.add_argument("--platforms", default="all")
    prepare.add_argument("--reuse-run", default="")
    prepare.add_argument("--reuse-metadata", type=Path)
    prepare.set_defaults(run=plan)
    pack = subparsers.add_parser("package")
    pack.add_argument("--directory", type=Path, required=True)
    pack.add_argument("--target", required=True)
    pack.add_argument("--tag", default="")
    pack.add_argument("--output", type=Path, required=True)
    pack.set_defaults(run=package)
    collect = subparsers.add_parser("assemble")
    collect.add_argument("--directory", type=Path, required=True)
    collect.add_argument("--tag", required=True)
    collect.add_argument("--output", type=Path, required=True)
    collect.add_argument("--notes", type=Path, required=True)
    collect.set_defaults(run=assemble)
    uploaded = subparsers.add_parser("verify-upload")
    uploaded.add_argument("--directory", type=Path, required=True)
    uploaded.add_argument("--metadata", type=Path, required=True)
    uploaded.set_defaults(run=verify_upload)
    args = parser.parse_args()
    args.run(args)


if __name__ == "__main__":
    main()
