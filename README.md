![Build Status](https://img.shields.io/github/actions/workflow/status/zirco-lang/zircon/build.yml?style=flat-square) ![Repo Size](https://img.shields.io/github/repo-size/zirco-lang/zircon?style=flat-square) ![open issues](https://img.shields.io/github/issues-raw/zirco-lang/zircon?style=flat-square) ![open PRs](https://img.shields.io/github/issues-pr-raw/zirco-lang/zircon?style=flat-square) ![license](https://img.shields.io/github/license/zirco-lang/zircon?style=flat-square)

<div align="center">

![Zirco banner](https://github.com/zirco-lang/assets/blob/main/png/wide-light.png)

# zircon: Zirco's toolchain installer

</div>

Zircon is a toolchain installer for the Zirco programming language. It allows you to easily install and manage different versions of the Zirco compiler, libraries, and tools on your system.

## Windows Support

Zircon does NOT support Windows. You must use WSL (Windows Subsystem for Linux) to run Zircon on Windows.

## Installation

### Prerequisites

Zircon requires the following dependencies to build and run Zirco:

-   **Rust** (install from [rustup.rs](https://rustup.rs/))
-   **LLVM 20** (**REQUIRED** - Zirco only works with LLVM 20.x)
-   **clang** (usually included with LLVM)
-   **Git**
-   **zstd, libssl, pkg-config** (for building on Linux)

#### Installing LLVM 20 and clang

**On macOS (Homebrew):**

```bash
brew install llvm@20
```

**On macOS (MacPorts):**

```bash
sudo port install llvm-20
```

**On Ubuntu/Debian:**

```bash
sudo apt install llvm-20 llvm-20-dev libpolly-20-dev clang-20 build-essential libssl-dev pkg-config libzstd-dev
```

**Note:** Zirco requires LLVM 20 specifically. Other versions will not work.

### Bootstrap Installation (Linux/macOS/WSL)

Run the bootstrap script to install Zircon (latest main branch):

```bash
curl -sSf https://raw.githubusercontent.com/zirco-lang/zircon/main/bootstrap.sh | bash
```

Or install a specific version:

```bash
curl -sSf https://raw.githubusercontent.com/zirco-lang/zircon/main/bootstrap.sh | bash -s v0.1.0
```

#### Manual Installation

Or manually:

```bash
# Clone and build Zircon
mkdir -p ~/.zircon/sources/zirco-lang
git clone https://github.com/zirco-lang/zircon.git ~/.zircon/sources/zirco-lang/zircon
cd ~/.zircon/sources/zirco-lang/zircon

# Optionally checkout a specific version
# git checkout v0.1.0

cargo build --release

# Create symlinks
ln -sf ~/.zircon/sources/zirco-lang/zircon ~/.zircon/self
mkdir -p ~/.zircon/bin
ln -sf ~/.zircon/sources/zirco-lang/zircon/target/release/zircon ~/.zircon/bin/zircon

# Add to PATH
export PATH="$HOME/.zircon/bin:$PATH"

# Run bootstrap
zircon _ bootstrap
```

### Add to Shell Profile

**Important:** You must add Zircon's bin directory to your PATH before using the `env` command.

#### Linux/macOS (Bash/Zsh)

Add this to your `~/.bashrc`, `~/.zshrc`, or equivalent:

```bash
export PATH="$HOME/.zircon/bin:$PATH"
```

Then, load the full environment (including `ZIRCO_INCLUDE_PATH`) by running:

```bash
source <(zircon env)
```

## Usage

### Install a Pre-Built Zirco Toolchain

For faster installation, you can download and install pre-built binaries from GitHub releases:

```bash
zircon install nightly
```

Install a specific release version:

```bash
zircon install v0.1.0
```

Install from a custom repository:

```bash
zircon install --zrc-repo myorg/zrc nightly
```

**Note:** Pre-built binaries are only available for certain platforms (Linux x64/ARM64, macOS x64/ARM64). If your platform is not supported, use `zircon build` instead.

### Build a Zirco Toolchain

Build the latest version from the main branch:

```bash
zircon build main
```

Build a specific version (tag):

```bash
zircon build v0.1.0
```

Build from a specific branch:

```bash
zircon build feat-145
```

Build from a custom repository:

```bash
zircon build --zrc-repo https://github.com/SomeFork/zrc main
```

### Switch Between Toolchains

```bash
zircon switch v0.1.0
```

### List Installed Toolchains

```bash
zircon list
```

### Delete a Toolchain

```bash
zircon delete v0.1.0
```

### Prune Unused Toolchains

Remove all toolchains except the currently active one:

```bash
zircon prune
```

Or skip the confirmation prompt:

```bash
zircon prune -y
```

### Update Zircon Itself

Update to the latest main branch:

```bash
zircon self update
```

Or update to a specific version:

```bash
zircon self update v0.1.0
zircon self update my-feature-branch
```

### Environment Configuration

Output shell environment variables:

```bash
zircon env
```

Or load them directly:

```bash
source <(zircon env)
```

This sets:

-   `PATH` to include `~/.zircon/bin`
-   `ZIRCO_INCLUDE_PATH` to point to the current toolchain's include directory

## Directory Structure

Zircon manages files in `~/.zircon` (or `%USERPROFILE%\.zircon` on Windows):

```text
~/.zircon
├── sources/
│   └── zirco-lang/
│       ├── zrc/          # Zirco compiler source
│       └── zircon/       # Zircon source (for self-updates)
├── toolchains/
│   ├── v0.1.0/
│   │   ├── bin/
│   │   │   └── zrc
│   │   └── include/
│   │       └── *.zh
│   └── current -> v0.1.0  # Symlink to active toolchain
├── self -> sources/zirco-lang/zircon  # Symlink to zircon source
└── bin/
    ├── zrc -> ../toolchains/current/bin/zrc
    └── zircon -> ../sources/zirco-lang/zircon/target/release/zircon
```

You can override the installation directory with the `ZIRCON_PREFIX` environment variable:

```bash
ZIRCON_PREFIX=/opt/zircon zircon build v0.1.0
```

## Platform Support

Zircon is designed to work on:

-   **Linux** ✓
-   **macOS** ✓
-   **Windows via WSL** ✓

## A Note on Stability

So that Zirco can continue to evolve at a rapid pace, there are **NO STABILITY GUARENTEES** on the current version of Zirco, `zrc`, and zircon.
