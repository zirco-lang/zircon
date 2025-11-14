#!/usr/bin/env bash
# Bootstrap script to build and install Zircon

set -euo pipefail

ZIRCON_REPO="https://github.com/zirco-lang/zircon.git"

echo "Checking for Git..."
if ! command -v git &>/dev/null; then
    echo "Git not found, please install Git and try again."
    exit 1
else
    echo "Git found: $(git --version)"
fi

echo "Looking for Rust..."
if ! command -v rustc &>/dev/null; then
    echo "Rust not found, installing via rustup..."
    echo "Please follow the prompts to complete the installation of the Rust toolchain."
    curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
    source "$HOME/.cargo/env"
fi
echo "Rust found: $(rustc --version)"

if [[ -d ~/.zircon ]]; then
    echo "Removing existing ~/.zircon directory to allow for a fresh install..."
    rm -rf ~/.zircon
fi

mkdir -p ~/.zircon
cd ~/.zircon

# Clone the Zircon repository
echo "Downloading Zircon source code..."
# TODO: In the future, we may want to do a shallow clone or download a specific release tarball
git clone "$ZIRCON_REPO" self
cd self

echo "Building Zircon..."
cargo build --release

# Create a symlink to the zircon binary in ~/.zircon/bin
mkdir -p ~/.zircon/bin
# ~/.zircon/bin/zircon is managed by this script. Later there will be other files in ~/.zircon/bin that Zircon itself manages.
ln -sf ~/.zircon/self/target/release/zircon ~/.zircon/bin/zircon

# This only adds to PATH for the duration of this script.
# Users will later be instructed to add this to their shell profile.
export PATH="$HOME/.zircon/bin:$PATH"

# Run the bootstrap command
echo ""
zircon _ bootstrap
