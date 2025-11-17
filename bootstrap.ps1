# Bootstrap script to build and install Zircon on Windows
#
# Usage: .\bootstrap.ps1 [refspec] [-Force]
#   refspec: Optional git reference (branch, tag, or commit) to checkout
#            Defaults to 'main' if not specified
#   -Force:  Skip confirmation prompt for deleting existing installation
#
# Or via direct download:
#   iwr -useb https://raw.githubusercontent.com/zirco-lang/zircon/main/bootstrap.ps1 -OutFile bootstrap.ps1; .\bootstrap.ps1
#   iwr -useb https://raw.githubusercontent.com/zirco-lang/zircon/main/bootstrap.ps1 -OutFile bootstrap.ps1; .\bootstrap.ps1 v0.1.0

param(
    [string]$RefSpec = "main",
    [switch]$Force
)

$ErrorActionPreference = "Stop"

$ZIRCON_REPO = "https://github.com/zirco-lang/zircon.git"
$ZIRCON_REF = $RefSpec

Write-Host "Checking for Git..."
try {
    $gitVersion = git --version 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "Git command failed with exit code $LASTEXITCODE"
    }
    Write-Host "Git found: $gitVersion"
} catch {
    Write-Error "Git not found, please install Git and try again."
    exit 1
}

Write-Host "Looking for Rust..."
try {
    $rustVersion = rustc --version 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "Rustc command failed"
    }
    Write-Host "Rust found: $rustVersion"
} catch {
    Write-Host "Rust not found, installing via rustup..."
    Write-Host "Downloading rustup-init.exe..."
    
    $rustupUrl = "https://win.rustup.rs/x86_64"
    $rustupInit = "$env:TEMP\rustup-init.exe"
    
    try {
        Invoke-WebRequest -Uri $rustupUrl -OutFile $rustupInit
        
        Write-Host "Running rustup installer..."
        & $rustupInit -y
        if ($LASTEXITCODE -ne 0) {
            throw "rustup-init failed with exit code $LASTEXITCODE"
        }
        
        # Add cargo to PATH for this session
        $env:Path += ";$env:USERPROFILE\.cargo\bin"
        
        Write-Host "Rust installed successfully"
    } catch {
        Write-Error "Failed to install Rust: $_"
        exit 1
    }
}

$zirconDir = "$env:USERPROFILE\.zircon"

if (Test-Path $zirconDir) {
    if (-not $Force) {
        Write-Host "Warning: Existing .zircon directory found at $zirconDir"
        $response = Read-Host "This will delete the existing installation. Continue? (y/N)"
        if ($response -ne 'y' -and $response -ne 'Y' -and $response -ne 'yes') {
            Write-Host "Installation cancelled."
            exit 0
        }
    }
    Write-Host "Removing existing .zircon directory..."
    Remove-Item -Recurse -Force $zirconDir
}

$sourcesDir = "$zirconDir\sources\zirco-lang"
New-Item -ItemType Directory -Force -Path $sourcesDir | Out-Null
Set-Location $sourcesDir

# Clone the Zircon repository
Write-Host "Downloading Zircon source code..."
git clone $ZIRCON_REPO zircon
if ($LASTEXITCODE -ne 0) {
    throw "git clone failed with exit code $LASTEXITCODE"
}

Set-Location zircon

# Checkout the specified reference
if ($ZIRCON_REF -ne "main") {
    Write-Host "Checking out reference: $ZIRCON_REF"
    git checkout $ZIRCON_REF
    if ($LASTEXITCODE -ne 0) {
        throw "git checkout failed with exit code $LASTEXITCODE"
    }
}

Write-Host "Building Zircon..."
cargo build --release
if ($LASTEXITCODE -ne 0) {
    throw "cargo build failed with exit code $LASTEXITCODE"
}

# Create symlink from self to sources/zirco-lang/zircon
$selfLink = "$zirconDir\self"
$zirconSource = "$sourcesDir\zircon"

# Remove existing link if present
if (Test-Path $selfLink) {
    Remove-Item -Force $selfLink
}

# Create directory junction (works without admin rights)
cmd /c mklink /J "$selfLink" "$zirconSource" 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) {
    throw "mklink failed with exit code $LASTEXITCODE"
}

# Create a symlink to the zircon binary in .zircon\bin
$binDir = "$zirconDir\bin"
New-Item -ItemType Directory -Force -Path $binDir | Out-Null

$zirconBin = "$zirconSource\target\release\zircon.exe"
$zirconLink = "$binDir\zircon.exe"

# Remove existing link if present  
if (Test-Path $zirconLink) {
    Remove-Item -Force $zirconLink
}

# Try to create a hard link (works without admin rights)
try {
    New-Item -ItemType HardLink -Path $zirconLink -Target $zirconBin | Out-Null
} catch {
    # Fallback to copy if hard link fails
    Copy-Item $zirconBin $zirconLink
}

# Add to PATH for this session
$env:Path = "$binDir;$env:Path"

Write-Host ""
Write-Host "Running bootstrap..."
& "$zirconLink" _ bootstrap
if ($LASTEXITCODE -ne 0) {
    throw "zircon bootstrap failed with exit code $LASTEXITCODE"
}

Write-Host ""
Write-Host "To permanently add Zircon to your PATH, run:"
Write-Host '  [Environment]::SetEnvironmentVariable("Path", "$env:Path;' + $binDir + '", "User")'
