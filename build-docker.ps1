# KTPAMXX Docker Build Script
$ErrorActionPreference = "Stop"

# Add Docker to PATH
$env:PATH = "C:\Program Files\Docker\Docker\resources\bin;" + $env:PATH

# Change to project directory
Set-Location "N:\Nein_\KTP Git Projects\KTPAMXX"

# Create output directory
New-Item -ItemType Directory -Force -Path "docker-output" | Out-Null

# Build the Docker image
Write-Host "Building Docker image..."
docker build -t ktpamxx-builder .

# Run the container with volume mounts
Write-Host "Running build..."
docker run --rm `
    -v "N:\Nein_\KTP Git Projects\KTPAMXX:/build/ktpamxx:ro" `
    -v "N:\Nein_\KTP Git Projects\KTPhlsdk:/build/ktphlsdk:ro" `
    -v "N:\Nein_\KTP Git Projects\KTPAMXX\docker-output:/build/output" `
    ktpamxx-builder

Write-Host ""
Write-Host "Build complete. Check docker-output/ for binaries."
