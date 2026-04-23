# setup_and_run.ps1
# This script automates the build and execution of the Graph Traversal Visualizer.

Write-Host "================================================="
Write-Host "   Graph Traversal Algorithm Visualizer Setup"
Write-Host "================================================="
Write-Host ""

# 1. Verify CMake is installed
if (-Not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Write-Host "ERROR: CMake is not installed or not added to your system PATH." -ForegroundColor Red
    Write-Host "Please install CMake from https://cmake.org/download/ and try again." -ForegroundColor Yellow
    Pause
    exit 1
}
Write-Host "[OK] CMake is installed." -ForegroundColor Green

# 2. Check Compiler environment (Ninja or Make)
if (-Not (Get-Command ninja -ErrorAction SilentlyContinue) -and -Not (Get-Command make -ErrorAction SilentlyContinue) -and -Not (Get-Command mingw32-make -ErrorAction SilentlyContinue)) {
    Write-Host "WARNING: Could not detect Ninja, Make, or Mingw32-make." -ForegroundColor Yellow
    Write-Host "Assuming a default compiler (e.g. Visual Studio) is configured." -ForegroundColor Yellow
} else {
    Write-Host "[OK] Compiler detected." -ForegroundColor Green
}

# 3. Create Build Directory
Write-Host "`n---> Step 1: Creating build directory..." -ForegroundColor Cyan
if (-Not (Test-Path -Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}
Set-Location -Path "build"

# 4. Run CMake Configuration
Write-Host "---> Step 2: Running CMake Configuration..." -ForegroundColor Cyan
cmake ..
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: CMake configuration failed. Ensure SFML is installed correctly." -ForegroundColor Red
    Write-Host "If using MSYS2, ensure you ran: pacman -S mingw-w64-ucrt-x86_64-sfml" -ForegroundColor Yellow
    Pause
    exit 1
}

# 5. Compile the Project
Write-Host "`n---> Step 3: Compiling the C++ Source Code..." -ForegroundColor Cyan
cmake --build .
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Compilation failed." -ForegroundColor Red
    Pause
    exit 1
}

# 6. Run the Executable
Write-Host "`n---> Step 4: Launching Application..." -ForegroundColor Green
if (Test-Path "PathVisualizer.exe") {
    Start-Process -FilePath ".\PathVisualizer.exe"
} elseif (Test-Path "Debug\PathVisualizer.exe") {
    Start-Process -FilePath ".\Debug\PathVisualizer.exe"
} elseif (Test-Path "Release\PathVisualizer.exe") {
    Start-Process -FilePath ".\Release\PathVisualizer.exe"
} else {
    Write-Host "WARNING: Could not locate the built executable automatically." -ForegroundColor Yellow
}

Write-Host "Setup Complete!" -ForegroundColor Green
Start-Sleep -Seconds 3
