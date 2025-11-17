#!/usr/bin/env bash
# Bootstrap script to build and install Zircon
#
# Usage: ./bootstrap.sh [refspec]
#   refspec: Optional git reference (branch, tag, or commit) to checkout
#            Defaults to 'main' if not specified
#
# Note: This script will fail immediately on any error due to 'set -e'

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
if ! git clone "$ZIRCON_REPO" zircon; then
    echo "Error: git clone failed with exit code $?"
    exit 1
fi
cd zircon

# Checkout the specified reference
if [[ "$ZIRCON_REF" != "main" ]]; then
    echo "Checking out reference: $ZIRCON_REF"
    if ! git checkout "$ZIRCON_REF"; then
        echo "Error: git checkout failed with exit code $?"
        exit 1
    fi
fi

echo "Building Zircon..."
if ! cargo build --release; then
    echo "Error: cargo build failed with exit code $?"
    exit 1
fi

# Create symlink from self to sources/zirco-lang/zircon
if ! ln -sf "$HOME/.zircon/sources/zirco-lang/zircon" "$HOME/.zircon/self"; then
    echo "Error: ln -sf (self symlink) failed with exit code $?"
    exit 1
fi

# Create a symlink to the zircon binary in ~/.zircon/bin
mkdir -p "$HOME/.zircon/bin"
# ~/.zircon/bin/zircon is managed by this script. Later there will be other files in ~/.zircon/bin that Zircon itself manages.
if ! ln -sf "$HOME/.zircon/sources/zirco-lang/zircon/target/release/zircon" "$HOME/.zircon/bin/zircon"; then
    echo "Error: ln -sf (bin symlink) failed with exit code $?"
    exit 1
fi

# This only adds to PATH for the duration of this script.
# Users will later be instructed to add this to their shell profile.
export PATH="$HOME/.zircon/bin:$PATH"

# Run the bootstrap command
echo ""
if ! zircon _ bootstrap; then
    echo "Error: zircon _ bootstrap failed with exit code $?"
    exit 1
fi
