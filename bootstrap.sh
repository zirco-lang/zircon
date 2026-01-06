#!/usr/bin/env bash
# Bootstrap script to build and install Zircon
#
# Usage: ./bootstrap.sh [ref]
#   ref: Optional git tag to pull built artifacts from
#        Defaults to 'nightly' if not specified

set -euo pipefail

ZIRCON_REPO="https://github.com/zirco-lang/zircon.git"
ZIRCON_REF="${1:-nightly}"

# Function to detect platform and architecture
detect_platform_arch() {
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
            exit 1
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
            exit 1
            ;;
    esac
    
    echo "${platform}-${architecture}"
}

echo "Checking for Git..."
if ! command -v git &>/dev/null; then
    echo "Git not found, please install Git and try again."
    exit 1
else
    echo "Git found: $(git --version)"
fi

if [[ -d "$HOME/.zircon" ]]; then
    echo "Removing existing ~/.zircon directory to allow for a fresh install..."
    rm -rf "$HOME/.zircon"
fi



    echo "Checking for prebuilt zircon binary..."
    
    # Detect platform and architecture
    if ! platform_arch=$(detect_platform_arch); then
        echo "Could not detect platform/architecture"
        exit 1
    fi
    
filename="zircon-${platform_arch}.tar.gz"
url="https://github.com/zirco-lang/zircon/releases/download/${ZIRCON_REF}/${filename}"

echo "Attempting to download prebuilt binary from: $url"

# Try to download the prebuilt binary using a secure temporary file
temp_file=$(mktemp)
if curl -fsSL "$url" -o "$temp_file" 2>/dev/null; then
    echo "✓ Prebuilt binary found! Extracting..."
    
    # Create a temporary extraction directory
    temp_extract_dir=$(mktemp -d)
    
    # Extract the archive to temporary directory with safety checks
    if tar -xzf "$temp_file" -C "$temp_extract_dir" 2>/dev/null; then
        echo "✓ Successfully extracted prebuilt zircon"
        
        # Move the extracted contents to self directory
        mkdir -p "$HOME/.zircon"
        rm -rf "$HOME/.zircon/self"
        mv "$temp_extract_dir" "$HOME/.zircon/self"
        
        # Make the binary executable
        chmod +x "$HOME/.zircon/self/bin/zircon"
        
        # Clean up
        rm -f "$temp_file"
    else
        echo "Failed to extract archive"
        rm -rf "$temp_extract_dir"
        rm -f "$temp_file"
        exit 1
    fi
else
    echo "Prebuilt binary not available for ${platform_arch}"
    rm -f "$temp_file" 2>/dev/null || true
    exit 1
fi
echo "✓ Zircon installed successfully"

# This only adds to PATH for the duration of this script.
# Users will later be instructed to add this to their shell profile.
export PATH="$HOME/.zircon/self/bin:$PATH"

echo "Next steps:"
echo "1. Add the following line to your shell profile (e.g., ~/.bashrc, ~/.zshrc):"
echo "   source <($HOME/.zircon/self/bin/zircon env)"
echo "2. Restart your terminal or run 'source ~/.bashrc' (or the appropriate command for your shell) to apply the changes."
echo "3. Run 'zircon install' to install the latest Zirco toolchain."
