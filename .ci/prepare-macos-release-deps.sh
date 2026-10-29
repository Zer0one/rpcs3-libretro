#!/bin/bash
set -euo pipefail

# This procedure replaces packages only on an ephemeral GitHub-hosted runner.
# A newer host can run Sequoia bottles without raising the release's minimum OS.
if [[ "${GITHUB_ACTIONS:-}" != true || "${RUNNER_ENVIRONMENT:-}" != github-hosted ]]; then
    echo 'Dependency replacement requires an ephemeral GitHub-hosted runner' >&2
    exit 1
fi
if [[ "$(uname -m)" != arm64 ]]; then
    echo 'Expected an ARM64 runner' >&2
    exit 1
fi
export HOMEBREW_NO_INSTALL_CLEANUP=1
export HOMEBREW_NO_AUTOREMOVE=1
export HOMEBREW_NO_AUTO_UPDATE=1

brew install ninja vulkan-headers >/dev/null
formulas=(zstd molten-vk llvm@22)
if ! python3 "$(dirname "$0")/fetch-macos-release-bottles.py" \
    --output macos-sequoia-bottles --tag arm64_sequoia "${formulas[@]}"; then
    echo 'Compatible bottles could not be verified; installed libraries are untouched' >&2
    exit 1
fi
bottles=()
while IFS= read -r bottle; do
    if [[ ! -s "$bottle" || "$bottle" != *.bottle*.tar.gz ]]; then
        echo "Missing verified bottle: $bottle" >&2
        exit 1
    fi
    bottles+=("$bottle")
done < macos-sequoia-bottles/paths.txt
if [[ "${#bottles[@]}" != "${#formulas[@]}" ]]; then
    echo 'Incomplete verified bottle set' >&2
    exit 1
fi
for formula in llvm@22 molten-vk zstd; do
    if brew list --versions "$formula" >/dev/null 2>&1; then
        brew uninstall --ignore-dependencies "$formula"
    fi
done
# Explicit local bottles prevent Homebrew choosing newer-host bottles instead.
for bottle in "${bottles[@]}"; do
    # Current Homebrew accepts local bottles only in developer/test mode.
    # Scope this to the already checksum-verified official bottle install.
    HOMEBREW_DEVELOPER=1 brew install --ignore-dependencies --force-bottle "$bottle"
done

llvm_prefix=$(brew --prefix llvm@22)
mvk_prefix=$(brew --prefix molten-vk)
zstd_prefix=$(brew --prefix zstd)
python3 "$(dirname "$0")/check-macos-deployment-target.py" --maximum 15.0 \
    --report macos-dependency-targets.json \
    "$llvm_prefix"/lib/libLLVM*.a \
    "$mvk_prefix/lib/libMoltenVK.dylib" "$zstd_prefix/lib/libzstd.dylib"

echo "LLVM=$llvm_prefix" >> "$GITHUB_ENV"
echo "BREW=$(brew --prefix)" >> "$GITHUB_ENV"
