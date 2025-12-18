#!/bin/bash
# Generate Arch Linux .pkg.tar.zst package

set -e

# Arguments
BUILD_DIR="$1"
PKG_NAME="$2"
PKG_VERSION="$3"
PKG_DESC="$4"

PKG_REL="1"
PKG_ARCH="$(uname -m)"
PKG_FILENAME="${PKG_NAME}-${PKG_VERSION}-${PKG_REL}-${PKG_ARCH}.pkg.tar.zst"

# Staging directory
STAGE_DIR="${BUILD_DIR}/_arch_pkg"

# Clean and create staging directory
rm -rf "${STAGE_DIR}"
mkdir -p "${STAGE_DIR}"

# Install to staging directory
cmake --install "${BUILD_DIR}" --prefix "${STAGE_DIR}/usr"

# Install autostart to /etc/xdg/autostart
mkdir -p "${STAGE_DIR}/etc/xdg/autostart"
cp "${STAGE_DIR}/usr/share/applications/plasma-clock-oled.desktop" "${STAGE_DIR}/etc/xdg/autostart/"

# Calculate installed size (in bytes)
PKG_SIZE=$(du -sb "${STAGE_DIR}" | cut -f1)

# Build timestamp
BUILD_DATE=$(date +%s)

# Generate .PKGINFO
cat > "${STAGE_DIR}/.PKGINFO" << EOF
pkgname = ${PKG_NAME}
pkgbase = ${PKG_NAME}
pkgver = ${PKG_VERSION}-${PKG_REL}
pkgdesc = ${PKG_DESC}
url = https://github.com/ustek-rfid/plasma-clock-oled
builddate = ${BUILD_DATE}
packager = CPack Arch Generator
size = ${PKG_SIZE}
arch = ${PKG_ARCH}
license = MIT
depend = qt6-base
depend = qt6-wayland
depend = qt6-svg
depend = layer-shell-qt
EOF

# Create install script for cache updates
cat > "${STAGE_DIR}/.INSTALL" << 'EOF'
post_install() {
    xdg-icon-resource forceupdate --theme hicolor 2>/dev/null || true
    update-desktop-database -q /usr/share/applications 2>/dev/null || true
}

post_upgrade() {
    post_install
}

post_remove() {
    post_install
}
EOF

# Generate .MTREE
cd "${STAGE_DIR}"

# List all files/directories to include
CONTENTS=".PKGINFO .INSTALL"
[ -d usr ] && CONTENTS="${CONTENTS} usr"
[ -d etc ] && CONTENTS="${CONTENTS} etc"

# Create MTREE
LANG=C bsdtar -czf .MTREE --format=mtree \
    --options='!all,use-set,type,uid,gid,mode,time,size,md5,sha256,link' \
    ${CONTENTS}

# Create the package
LANG=C bsdtar -cf - .MTREE ${CONTENTS} | zstd -c -T0 -19 > "${BUILD_DIR}/${PKG_FILENAME}"

echo "Created: ${BUILD_DIR}/${PKG_FILENAME}"
