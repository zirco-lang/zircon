# Bootstrap script to build and install Zircon on Windows
#
# Usage: .\bootstrap.ps1 [refspec]
#   refspec: Optional git reference (branch, tag, or commit) to checkout
#            Defaults to 'main' if not specified
#
# Or via direct download:
#   iwr -useb https://raw.githubusercontent.com/zirco-lang/zircon/main/bootstrap.ps1 | iex
#   iwr -useb https://raw.githubusercontent.com/zirco-lang/zircon/main/bootstrap.ps1 | iex -ArgumentList "v0.1.0"

param(
    [string]$RefSpec = "main"
)

$ErrorActionPreference = "Stop"

$ZIRCON_REPO = "https://github.com/zirco-lang/zircon.git"
$ZIRCON_REF = $RefSpec

Write-Host "Checking for Git..."
try {
    $gitVersion = git --version
    Write-Host "Git found: $gitVersion"
} catch {
    Write-Error "Git not found, please install Git and try again."
    exit 1
}

Write-Host "Looking for Rust..."
try {
    $rustVersion = rustc --version
    Write-Host "Rust found: $rustVersion"
} catch {
    Write-Host "Rust not found, installing via rustup..."
    Write-Host "Downloading rustup-init.exe..."
    
    $rustupUrl = "https://win.rustup.rs/x86_64"
    $rustupInit = "$env:TEMP\rustup-init.exe"
    
    Invoke-WebRequest -Uri $rustupUrl -OutFile $rustupInit
    
    Write-Host "Running rustup installer..."
    & $rustupInit -y
    
    # Add cargo to PATH for this session
    $env:Path += ";$env:USERPROFILE\.cargo\bin"
    
    Write-Host "Rust installed successfully"
}

$zirconDir = "$env:USERPROFILE\.zircon"

if (Test-Path $zirconDir) {
    Write-Host "Removing existing .zircon directory to allow for a fresh install..."
    Remove-Item -Recurse -Force $zirconDir
}

$sourcesDir = "$zirconDir\sources\zirco-lang"
New-Item -ItemType Directory -Force -Path $sourcesDir | Out-Null
Set-Location $sourcesDir

# Clone the Zircon repository
Write-Host "Downloading Zircon source code..."
git clone $ZIRCON_REPO zircon
Set-Location zircon

# Checkout the specified reference
if ($ZIRCON_REF -ne "main") {
    Write-Host "Checking out reference: $ZIRCON_REF"
    git checkout $ZIRCON_REF
}

Write-Host "Building Zircon..."
cargo build --release

# Create symlink from self to sources/zirco-lang/zircon
$selfLink = "$zirconDir\self"
$zirconSource = "$sourcesDir\zircon"

# Remove existing link if present
if (Test-Path $selfLink) {
    Remove-Item -Force $selfLink
}

# Create directory junction (works without admin rights)
cmd /c mklink /J "$selfLink" "$zirconSource" | Out-Null

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

Write-Host ""
Write-Host "To permanently add Zircon to your PATH, run:"
Write-Host '  [Environment]::SetEnvironmentVariable("Path", "$env:Path;' + $binDir + '", "User")'
