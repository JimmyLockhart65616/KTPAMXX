#!/bin/bash
set -e

echo "========================================"
echo "KTPAMXX Docker Build for Ubuntu 22.04"
echo "========================================"

# Copy source to writable location
cp -r /build/ktpamxx /tmp/ktpamxx
cd /tmp/ktpamxx

# Set up Python virtual environment for AMBuild
VENV_DIR="/tmp/ambuild-venv"
python3 -m venv "$VENV_DIR"
source "$VENV_DIR/bin/activate"

# Install AMBuild
echo "Installing AMBuild..."
cd support/ambuild
pip install . -q
cd ../..

# Set HLSDK and METAMOD paths
export HLSDK="/build/ktphlsdk"
export METAMOD="/build/metamod-am"

echo "HLSDK: $HLSDK"
echo "METAMOD: $METAMOD"
echo "Checking metamod path..."
ls -la /build/metamod-am/metamod/ 2>&1 | head -5

# Configure and build
echo "Configuring build..."
# Force clean configure (delete cached config)
rm -rf obj-linux 2>/dev/null || true
python3 configure.py --enable-optimize --no-mysql --no-plugins

echo "Building..."
cd obj-linux
ambuild
cd ..

# Check result and copy to output
if [ -f "obj-linux/packages/base/addons/ktpamx/dlls/ktpamx_i386.so" ]; then
    echo ""
    echo "========================================"
    echo "BUILD SUCCESS!"
    echo "========================================"

    # Copy outputs
    mkdir -p /build/output/dlls /build/output/modules
    cp obj-linux/packages/base/addons/ktpamx/dlls/*.so /build/output/dlls/ 2>/dev/null || true
    cp obj-linux/packages/dod/addons/ktpamx/modules/*.so /build/output/modules/ 2>/dev/null || true
    cp obj-linux/packages/base/addons/ktpamx/modules/*.so /build/output/modules/ 2>/dev/null || true

    echo ""
    echo "Output files in docker-output/:"
    ls -lh /build/output/dlls/ 2>/dev/null || true
    ls -lh /build/output/modules/ 2>/dev/null || true
else
    echo "BUILD FAILED!"
    exit 1
fi
