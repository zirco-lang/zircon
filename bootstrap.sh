#!/usr/bin/env bash
# Bootstrap script to build and install Zircon
#
# Usage: ./bootstrap.sh [refspec]
#   refspec: Optional git reference (branch, tag, or commit) to checkout
#            Defaults to 'main' if not specified

set -euo pipefail

ZIRCON_REPO="https://github.com/zirco-lang/zircon.git"
ZIRCON_REF="${1:-main}"

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

if [[ -d "$HOME/.zircon" ]]; then
    echo "Removing existing ~/.zircon directory to allow for a fresh install..."
    rm -rf "$HOME/.zircon"
fi

mkdir -p "$HOME/.zircon/sources/zirco-lang"
cd "$HOME/.zircon/sources/zirco-lang"

# Clone the Zircon repository
echo "Downloading Zircon source code..."
git clone "$ZIRCON_REPO" zircon
cd zircon

# Checkout the specified reference
if [[ "$ZIRCON_REF" != "main" ]]; then
    echo "Checking out reference: $ZIRCON_REF"
    git checkout "$ZIRCON_REF"
fi

echo "Building Zircon..."
cargo build --release

# Create symlink from self to sources/zirco-lang/zircon
ln -sf "$HOME/.zircon/sources/zirco-lang/zircon" "$HOME/.zircon/self"

# Create a symlink to the zircon binary in ~/.zircon/bin
mkdir -p "$HOME/.zircon/bin"
# ~/.zircon/bin/zircon is managed by this script. Later there will be other files in ~/.zircon/bin that Zircon itself manages.
ln -sf "$HOME/.zircon/sources/zirco-lang/zircon/target/release/zircon" "$HOME/.zircon/bin/zircon"

# This only adds to PATH for the duration of this script.
# Users will later be instructed to add this to their shell profile.
export PATH="$HOME/.zircon/bin:$PATH"

# Run the bootstrap command
echo ""
zircon _ bootstrap
