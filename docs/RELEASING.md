# Automated Releases

The `Build And Publish Release` workflow builds Linux x86_64/ARM64, Windows x86_64, macOS ARM64/x86_64, and Android ARM64/x86_64 through the reusable `Build libretro core` workflow. Android packages remain experimental. A successful remote build establishes compilation and artifact checks, not gameplay or physical-device behavior.

## Publish A New Version

1. Complete validation on the feature branch and squash the related changes into `libretro` as required by `AGENTS.md`. Keep `main` aligned without rewriting either branch's published history.
2. Record user-facing changes in `CHANGELOG.md` under `## vMAJOR.MINOR.PATCH.BUILD`. The first independent release is `v0.1.0.1`; increment the final component for the next ordinary release.
3. Push the reviewed commit to `origin/main` and `origin/libretro`.
4. Create an annotated version tag on that commit and push the tag. Tag pushes start the remote release workflow automatically.

```sh
git tag -a v0.1.0.2 -m 'RPCS3 Libretro v0.1.0.2' <reviewed-main-commit>
git push origin refs/tags/v0.1.0.2
```

The upstream RPCS3 version in `rpcs3/libretro/upstream.version` changes only with a full upstream merge. It remains the core's engine version; the fork commit in the startup log identifies the numbered release. A fork release tag does not replace the engine build number.

## Complete Or Retry An Existing Release

From Actions, run **Build And Publish Release** on `main`, setting `release_tag` to the existing tag. For example:

```sh
gh workflow run release.yml --repo Zer0one/rpcs3-libretro --ref main -f release_tag=v0.1.0.1
```

The source tag must already exist and name a commit contained in `origin/main`. It is never moved or rewritten. Current CI tools are checked out separately from the tagged emulator source, so a packaging/build-procedure repair can rebuild an existing release without silently substituting newer emulation code. `BUILD_INFO.json` records both the source commit and the workflow commit/run.

An empty `release_tag` performs verification builds without publishing. The separate **Build libretro core** manual workflow still permits selected platform groups and symbol retention. Windows/Android failures may be tolerated in that diagnostic workflow; they are mandatory failures in a release run.

## Publication Gates

Publication waits for all seven builds. Every archive contains the canonical `rpcs3_libretro.so`, `.dll` or `.dylib`, matching `.info`, any packaged compression dependency, license, source commit, build metadata and payload checksums. The macOS packages reference the frontend's `@rpath/MoltenVK`; they do not include or install a Vulkan driver. Read each package's metadata for its minimum macOS version.

The package helper checks machine architecture, Libretro entry-point names, embedded source identity, tag/commit alignment, expected payload files and checksums. The publishing job rejects a missing or mismatched platform package, creates aggregate `BUILD_INFO.json` and `SHA256SUMS`, and checks GitHub's uploaded-asset digests. It then publishes an ordinary **Latest** release. Existing release assets with the same names are refreshed only after the complete build and package gates pass. No personal access token is needed: builds use read permissions and only the publishing job requests `contents: write`.

Sync remains the default MoltenVK submission mode. Async and the previously recorded 3D/pause-resume texture question remain subject to the runtime validation limits in `MACOS_PERFORMANCE.md` and `MACOS_VALIDATION.md`.
