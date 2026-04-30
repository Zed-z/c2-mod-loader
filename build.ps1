param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",
    [ValidateSet("EU","US","DEMO")]
    [string]$Version = "US",
    [switch]$Clean,
    [switch]$Deploy,
    [switch]$Package,
    [switch]$Launch,
    [switch]$Help
)

if ($Help) {
    Write-Host @"
Arguments: [-Configuration <Debug|Release>] [-Version <EU|US|DEMO>] [-Clean] [-Deploy] [-Package] [-Launch]
    -Configuration : Build configuration (Debug or Release). Default: Release
    -Version      : Croc 2 game version to target (EU, US, DEMO). Default: US
    -Clean        : Clean the build directory before building
    -Deploy       : Copy mod files to Croc 2 directory
    -Package      : Create package folder + zip
    -Launch       : Deploy and launch Croc 2 with mods
    -Help         : Show this help message
"@
    exit 0
}

# Locate Visual Studio Build Tools / VS installation with C++ tools
$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vsWhere)) {
    Write-Host "ERROR: vswhere.exe not found." -ForegroundColor Red
    exit 1
}

$vsInstallPath = & $vsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsInstallPath) {
    Write-Host "ERROR: MSVC x86/x64 build tools not found." -ForegroundColor Red
    exit 1
}

$vsDevCmd = Join-Path $vsInstallPath "Common7\Tools\VsDevCmd.bat"
if (-not (Test-Path $vsDevCmd)) {
    Write-Host "ERROR: VsDevCmd.bat not found at $vsDevCmd" -ForegroundColor Red
    exit 1
}

$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDir = Join-Path $projectRoot "build"
$gameDir = Join-Path $projectRoot "Croc2\$Version"

Write-Host "Building C2ModLoader with CMake + MSVC" -ForegroundColor Cyan
Write-Host "Configuration: $Configuration" -ForegroundColor Cyan
Write-Host "Version: $Version" -ForegroundColor Cyan
Write-Host "Game Directory: $gameDir" -ForegroundColor Cyan
Write-Host "VS DevCmd: $vsDevCmd" -ForegroundColor Cyan
Write-Host "Build Directory: $buildDir" -ForegroundColor Cyan

# Clean build directory
if ($Clean) {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    if (Test-Path $buildDir) {
        Remove-Item -Recurse -Force $buildDir
    }
}

# Create build directory
if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

# Configure + build
Write-Host "Configuring and building with MSVC x86..." -ForegroundColor Yellow
$cmakeGameDir = "`"$gameDir`""
$cmakeModsDir = "`"$($gameDir)\mods`""
$cmakeArgs = "-S `"$projectRoot`" -B `"$buildDir`" -G Ninja -DCMAKE_BUILD_TYPE=$Configuration -DCROC2_GAME_DIR=$cmakeGameDir -DCROC2_MODS_DIR=$cmakeModsDir"
$cmd = "call `"$vsDevCmd`" -arch=x86 -host_arch=x64 >nul && cmake $cmakeArgs && cmake --build `"$buildDir`""
Write-Host "Running via cmd.exe: $cmd" -ForegroundColor Gray
cmd /c $cmd
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: CMake configure/build failed" -ForegroundColor Red
    exit 1
}

Write-Host "Build completed successfully." -ForegroundColor Green
Write-Host "Output files in: $buildDir\Release" -ForegroundColor Green

# List output files
Write-Host "`nGenerated files:" -ForegroundColor Cyan
if (Test-Path "$buildDir\Release\*.asi") {
    Get-ChildItem "$buildDir\Release\*.asi" | ForEach-Object {
        Write-Host "  - $($_.Name)"
    }
}

# Deploy and/or Launch
if ($Deploy -or $Launch) {
    Write-Host "`nRunning CMake Deploy target..." -ForegroundColor Yellow
    $cmd = "call `"$vsDevCmd`" -arch=x86 -host_arch=x64 >nul && cmake --build `"$buildDir`" --target Deploy"
    cmd /c $cmd
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Deploy target failed" -ForegroundColor Red
        exit 1
    }
    Write-Host "Deploy completed successfully!" -ForegroundColor Green
    
    if ($Launch) {
        Write-Host "`nRunning CMake Launch target..." -ForegroundColor Yellow
        $cmd = "call `"$vsDevCmd`" -arch=x86 -host_arch=x64 >nul && cmake --build `"$buildDir`" --target Launch"
        cmd /c $cmd
        # Don't treat game exit code as error - user may close with Alt+F4
        Write-Host "Game closed." -ForegroundColor Cyan
    }
}

# Package
if ($Package) {
    Write-Host "`nRunning CMake Package target..." -ForegroundColor Yellow
    $cmd = "call `"$vsDevCmd`" -arch=x86 -host_arch=x64 >nul && cmake --build `"$buildDir`" --target Package"
    cmd /c $cmd
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Package target failed" -ForegroundColor Red
        exit 1
    }
    Write-Host "Package created successfully!" -ForegroundColor Green
    Write-Host "Folder: $buildDir\package" -ForegroundColor Green
    Write-Host "Zip: $buildDir\package.zip" -ForegroundColor Green
}

exit 0
