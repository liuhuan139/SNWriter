#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
PACKAGE_DIR="${SCRIPT_DIR}/package"
DEB_PACKAGE_DIR="${PACKAGE_DIR}/SNWriter"

echo "=== SNWriter Deb Package Builder ==="

# Clean previous builds
echo "[1/7] Cleaning previous builds..."
rm -rf "${BUILD_DIR}"
rm -rf "${PACKAGE_DIR}"
mkdir -p "${BUILD_DIR}"
mkdir -p "${DEB_PACKAGE_DIR}"

# Build the application
echo "[2/7] Building application..."
cd "${BUILD_DIR}"
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Create package directory structure
echo "[3/7] Creating package structure..."
mkdir -p "${DEB_PACKAGE_DIR}/opt/SNWriter"
mkdir -p "${DEB_PACKAGE_DIR}/opt/SNWriter/lib"
mkdir -p "${DEB_PACKAGE_DIR}/usr/local/bin"
mkdir -p "${DEB_PACKAGE_DIR}/DEBIAN"

# Copy executable
echo "[4/7] Copying executable..."
cp "${BUILD_DIR}/SNWriter" "${DEB_PACKAGE_DIR}/opt/SNWriter/"

# Copy required libraries
echo "[5/7] Collecting dependencies..."
"${SCRIPT_DIR}/scripts/copy_libs.sh" "${BUILD_DIR}/SNWriter" "${DEB_PACKAGE_DIR}/opt/SNWriter/lib"

# Create wrapper script
echo "[6/7] Creating wrapper script..."
cat > "${DEB_PACKAGE_DIR}/opt/SNWriter/SNWriter.sh" << 'EOF'
#!/bin/bash
# 解析符号链接，获取脚本真实路径
SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do
    DIR="$(cd -P "$(dirname "$SOURCE")" && pwd)"
    SOURCE="$(readlink "$SOURCE")"
    [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
done
SCRIPT_DIR="$(cd -P "$(dirname "$SOURCE")" && pwd)"

export LD_LIBRARY_PATH="${SCRIPT_DIR}/lib:${LD_LIBRARY_PATH}"
exec "${SCRIPT_DIR}/SNWriter" "$@"
EOF
chmod +x "${DEB_PACKAGE_DIR}/opt/SNWriter/SNWriter.sh"

# Create direct wrapper in /usr/local/bin instead of symlink
echo "#!/bin/bash" > "${DEB_PACKAGE_DIR}/usr/local/bin/SNWriter"
echo "exec /opt/SNWriter/SNWriter.sh \"\$@\"" >> "${DEB_PACKAGE_DIR}/usr/local/bin/SNWriter"
chmod +x "${DEB_PACKAGE_DIR}/usr/local/bin/SNWriter"

# Create DEBIAN control file
echo "[7/7] Creating DEBIAN control file..."
cat > "${DEB_PACKAGE_DIR}/DEBIAN/control" << EOF
Package: snwriter
Version: 1.0.0
Section: utils
Priority: optional
Architecture: amd64
Depends: adb
Maintainer: SNWriter Team
Description: SN Writer Tool
 Tool for modifying proinfo partition on Android devices.
EOF

# Set permissions
chmod 755 "${DEB_PACKAGE_DIR}/DEBIAN"
chmod 644 "${DEB_PACKAGE_DIR}/DEBIAN/control"
chmod +x "${DEB_PACKAGE_DIR}/opt/SNWriter/SNWriter"

# Build the deb package
echo ""
echo "Building deb package..."
cd "${PACKAGE_DIR}"
fakeroot dpkg-deb --build SNWriter

mv SNWriter.deb "${SCRIPT_DIR}/SNWriter_1.0.0_amd64.deb"

echo ""
echo "====================================="
echo "Deb package created successfully!"
echo "Package: ${SCRIPT_DIR}/SNWriter_1.0.0_amd64.deb"
echo "====================================="
