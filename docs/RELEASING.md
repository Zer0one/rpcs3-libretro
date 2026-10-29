# Automated Releases

The `Build And Publish Release` workflow builds the targets recorded in the source tag's `.ci/release-targets.json` through the reusable `Build libretro core` workflow. Future releases currently require five desktop packages: Linux x86_64/ARM64, Windows x86_64, and macOS ARM64/x86_64. Android publication is paused until further notice. Tags predating the manifest, including v0.1.0.1 and v0.1.0.2, retain their seven-platform policy for reproducible recovery. A successful remote build establishes compilation and artifact checks, not gameplay or physical-device behavior.

## Publish A New Version

1. Complete review and validation on `main`, following the maintainer's workflow for this independent fork. Create another branch only when explicitly requested. Do not rewrite published history or modify the developer-owned `AGENTS.md`/`CLAUDE.md` files.
2. Record user-facing changes in `CHANGELOG.md` under `## vMAJOR.MINOR.PATCH.BUILD`. The first independent release is `v0.1.0.1`; increment the final component for the next ordinary release.
3. Push the reviewed commit to `origin/main`.
4. Create an annotated version tag on that commit and push the tag. Tag pushes start the remote release workflow automatically.

```sh
git tag -a v0.1.0.3 -m 'RPCS3 Libretro v0.1.0.3' <reviewed-main-commit>
git push origin refs/tags/v0.1.0.3
```

The upstream RPCS3 version in `rpcs3/libretro/upstream.version` changes only with a full upstream merge. It remains the core's engine version; the fork commit in the startup log identifies the numbered release. A fork release tag does not replace the engine build number.

## Complete Or Retry An Existing Release

From Actions, run **Build And Publish Release** on `main`, setting `release_tag` to the existing tag. For example:

```sh
gh workflow run release.yml --repo Zer0one/rpcs3-libretro --ref main -f release_tag=v0.1.0.1
```

The source tag must already exist and name a commit contained in `origin/main`. It is never moved or rewritten. Current CI tools are checked out separately from the tagged emulator source, so a packaging/build-procedure repair can rebuild an existing release without silently substituting newer emulation code. `BUILD_INFO.json` records both the source commit and the workflow commit/run.

If only one platform group fails, select that group in `platforms` and provide the completed release run in `reuse_run_id`. For example, rebuild Windows for the legacy seven-target v0.1.0.1 release while retaining its six successful packages:

```sh
gh workflow run release.yml --repo Zer0one/rpcs3-libretro --ref main \
  -f release_tag=v0.1.0.1 -f platforms=windows -f reuse_run_id=40225618032
```

Recovery accepts only a completed run of this repository's release workflow from a commit contained in `origin/main`. The publisher combines previous and rebuilt artifacts, then applies the same mandatory source/architecture/checksum gates for every target required by that source tag. It does not publish a Windows-only release. Tag pushes select all platforms required by the tagged source policy.

An empty `release_tag` performs verification builds without publishing. The separate **Build libretro core** manual workflow still permits selected platform groups and symbol retention. Windows/Android failures may be tolerated in that diagnostic workflow; every platform selected by the source tag is mandatory in a release run. Android cannot be selected for release tags whose manifest excludes it.

## Publication Gates

The separate diagnostic `Build libretro core` workflow accepts `macos_arch` without publishing. For example, `gh workflow run libretro.yml --ref main -f platforms=macos -f macos_arch=arm64 -f keep_symbols=false` verifies the ARM64 build recipe on its own.

For a single macOS architecture, recovery can set `macos_arch=arm64` or `x86_64` together with `platforms=macos` and a completed `reuse_run_id`. The other architecture and platforms are restored from that run; all five desktop packages remain mandatory. For example:

```sh
gh workflow run release.yml --repo Zer0one/rpcs3-libretro --ref main \
  -f release_tag=v0.1.0.3 -f platforms=macos -f macos_arch=arm64 -f reuse_run_id=40727977617
```

The ARM64 job uses the macOS 26 hosted pool to avoid the observed macOS 15 runner-capacity failure. Its linked Homebrew libraries are selected explicitly from the public formula metadata as `arm64_sequoia` bottles, downloaded from Homebrew’s GHCR registry, checksum-verified as a complete set, and installed only on an ephemeral GitHub-hosted runner. A failed selection or download stops before installed libraries are replaced; it does not fall back to newer-host bottles. The dependency guard rejects any LLVM archive, MoltenVK or packaged zstd library requiring newer than macOS 15. The core still builds with deployment target 15.0; the newer host does not raise the advertised minimum. The package records the separate workflow and immutable source commits. Intel keeps its macOS 15 host.

Current Homebrew requires developer/test mode to load a local bottle. The helper sets `HOMEBREW_DEVELOPER=1` only on the install command for each already verified official bottle; it does not export that mode to other commands or change the controlling Mac's Homebrew configuration.

Publication waits for every build required by the tagged source policy. The manifest is read from that immutable source commit, not the workflow checkout, so recovery of old releases retains its original platform coverage. Every archive contains the canonical `rpcs3_libretro.so`, `.dll` or `.dylib`, matching `.info`, any packaged compression dependency, license, source commit, build metadata and payload checksums. The macOS packages reference the frontend's `@rpath/MoltenVK`; they do not include or install a Vulkan driver. Read each package's metadata for its minimum macOS version.

The package helper checks machine architecture, Libretro entry-point names, embedded source identity, tag/commit alignment, expected payload files and checksums. The publishing job rejects a missing or mismatched platform package, creates aggregate `BUILD_INFO.json` and `SHA256SUMS`, and checks GitHub's uploaded-asset digests. It then publishes an ordinary **Latest** release. Existing release assets with the same names are refreshed only after the complete build and package gates pass. No personal access token is needed: builds use read permissions and only the publishing job requests `contents: write`.

Sync remains the default MoltenVK submission mode. Async and the previously recorded 3D/pause-resume texture question remain subject to the runtime validation limits in `MACOS_PERFORMANCE.md` and `MACOS_VALIDATION.md`.
