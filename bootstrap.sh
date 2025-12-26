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

# Function to detect platform and architecture
detect_platform_arch() {
    local os
    local arch
    local platform
    local architecture
    os=$(uname -s | tr '[:upper:]' '[:lower:]')
    arch=$(uname -m)
    
    case "$os" in
        linux*)
            platform="linux"
            ;;
        darwin*)
            platform="macos"
            ;;
        *)
            echo "Unsupported platform: $os"
            return 1
            ;;
    esac
    
    case "$arch" in
        x86_64)
            architecture="x64"
            ;;
        aarch64|arm64)
            architecture="arm64"
            ;;
        *)
            echo "Unsupported architecture: $arch"
            return 1
            ;;
    esac
    
    echo "${platform}-${architecture}"
}

# Function to try downloading and installing prebuilt zircon
try_install_prebuilt() {
    local ref="$1"
    
    echo "Checking for prebuilt zircon binary..."
    
    # Detect platform and architecture
    local platform_arch
    if ! platform_arch=$(detect_platform_arch); then
        echo "Could not detect platform/architecture, will build from source"
        return 1
    fi
    
    local filename="zircon-${platform_arch}.tar.gz"
    local url="https://github.com/zirco-lang/zircon/releases/download/${ref}/${filename}"
    
    echo "Attempting to download prebuilt binary from: $url"
    
    # Try to download the prebuilt binary using a secure temporary file
    local temp_file
    temp_file=$(mktemp "/tmp/zircon-${platform_arch}.XXXXXX.tar.gz")
    if curl -fsSL "$url" -o "$temp_file" 2>/dev/null; then
        echo "✓ Prebuilt binary found! Extracting..."
        
        # Create the self directory
        mkdir -p "$HOME/.zircon/self"
        
        # Extract the archive to self directory with safety checks
        if tar -xzf "$temp_file" --one-top-level="$HOME/.zircon/self" --strip-components=1 2>/dev/null; then
            echo "✓ Successfully extracted prebuilt zircon"
            
            # Make the binary executable
            chmod +x "$HOME/.zircon/self/bin/zircon"
            
            # Create the bin directory and symlink
            mkdir -p "$HOME/.zircon/bin"
            ln -sf "$HOME/.zircon/self/bin/zircon" "$HOME/.zircon/bin/zircon"
            
            # Clean up
            rm -f "$temp_file"
            
            return 0
        else
            echo "Failed to extract archive, will build from source"
            rm -f "$temp_file"
            return 1
        fi
    else
        echo "Prebuilt binary not available for ${platform_arch}, will build from source"
        rm -f "$temp_file" 2>/dev/null || true
        return 1
    fi
}

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
    # shellcheck source=/dev/null
    source "$HOME/.cargo/env"
fi
echo "Rust found: $(rustc --version)"

if [[ -d "$HOME/.zircon" ]]; then
    echo "Removing existing ~/.zircon directory to allow for a fresh install..."
    rm -rf "$HOME/.zircon"
fi

# Try to install prebuilt binary first
if try_install_prebuilt "$ZIRCON_REF"; then
    echo "✓ Prebuilt zircon installed successfully"
else
    # Fall back to building from source
    echo ""
    echo "Building zircon from source..."
    
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
