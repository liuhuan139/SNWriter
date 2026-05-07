#!/bin/bash

set -e

if [ $# -ne 2 ]; then
    echo "Usage: $0 <executable> <target_lib_dir>"
    exit 1
fi

EXECUTABLE="$1"
TARGET_LIB_DIR="$2"

mkdir -p "${TARGET_LIB_DIR}"

echo "Collecting dependencies for ${EXECUTABLE}..."

# Function to copy a library and its dependencies
copy_lib() {
    local lib="$1"
    local lib_name=$(basename "${lib}")

    if [ -f "${TARGET_LIB_DIR}/${lib_name}" ]; then
        return
    fi

    if [ -f "${lib}" ]; then
        echo "  Copying ${lib_name}"
        cp "${lib}" "${TARGET_LIB_DIR}/"

        # Copy dependencies of this library
        local deps=$(ldd "${lib}" 2>/dev/null | grep "=> /" | awk '{print $3}' | grep -v '^$')
        for dep in $deps; do
            copy_lib "${dep}"
        done
    fi
}

# Get direct dependencies of the executable
DIRECT_LIBS=$(ldd "${EXECUTABLE}" 2>/dev/null | grep "=> /" | awk '{print $3}' | grep -v '^$')

for lib in $DIRECT_LIBS; do
    copy_lib "${lib}"
done

# Additional commonly needed GTK/Glib libraries that might be missed
COMMON_LIBS=(
    "/lib/x86_64-linux-gnu/libgtk-3.so.0"
    "/lib/x86_64-linux-gnu/libgdk-3.so.0"
    "/lib/x86_64-linux-gnu/libglib-2.0.so.0"
    "/lib/x86_64-linux-gnu/libgobject-2.0.so.0"
    "/lib/x86_64-linux-gnu/libgio-2.0.so.0"
    "/lib/x86_64-linux-gnu/libgmodule-2.0.so.0"
    "/lib/x86_64-linux-gnu/libgthread-2.0.so.0"
    "/lib/x86_64-linux-gnu/libpango-1.0.so.0"
    "/lib/x86_64-linux-gnu/libpangocairo-1.0.so.0"
    "/lib/x86_64-linux-gnu/libcairo.so.2"
    "/lib/x86_64-linux-gnu/libcairo-gobject.so.2"
    "/lib/x86_64-linux-gnu/libatk-1.0.so.0"
    "/lib/x86_64-linux-gnu/libatk-bridge-2.0.so.0"
    "/lib/x86_64-linux-gnu/libgdk_pixbuf-2.0.so.0"
    "/lib/x86_64-linux-gnu/libgtkmm-3.0.so.1"
    "/lib/x86_64-linux-gnu/libgdkmm-3.0.so.1"
    "/lib/x86_64-linux-gnu/libglibmm-2.4.so.1"
    "/lib/x86_64-linux-gnu/libgiomm-2.4.so.1"
    "/lib/x86_64-linux-gnu/libpangomm-1.4.so.1"
    "/lib/x86_64-linux-gnu/libcairomm-1.0.so.1"
    "/lib/x86_64-linux-gnu/libatkmm-1.6.so.1"
    "/lib/x86_64-linux-gnu/libsigc-2.0.so.0"
    "/lib/x86_64-linux-gnu/libfreetype.so.6"
    "/lib/x86_64-linux-gnu/libfontconfig.so.1"
    "/lib/x86_64-linux-gnu/libpng16.so.16"
    "/lib/x86_64-linux-gnu/libjpeg.so.8"
    "/lib/x86_64-linux-gnu/libtiff.so.5"
    "/lib/x86_64-linux-gnu/libX11.so.6"
    "/lib/x86_64-linux-gnu/libXext.so.6"
    "/lib/x86_64-linux-gnu/libXrender.so.1"
    "/lib/x86_64-linux-gnu/libXinerama.so.1"
    "/lib/x86_64-linux-gnu/libXi.so.6"
    "/lib/x86_64-linux-gnu/libXrandr.so.2"
    "/lib/x86_64-linux-gnu/libXcursor.so.1"
    "/lib/x86_64-linux-gnu/libXcomposite.so.1"
    "/lib/x86_64-linux-gnu/libXdamage.so.1"
    "/lib/x86_64-linux-gnu/libXfixes.so.3"
    "/lib/x86_64-linux-gnu/libxcb.so.1"
    "/lib/x86_64-linux-gnu/libxcb-render.so.0"
    "/lib/x86_64-linux-gnu/libxcb-shm.so.0"
    "/lib/x86_64-linux-gnu/libICE.so.6"
    "/lib/x86_64-linux-gnu/libSM.so.6"
    "/lib/x86_64-linux-gnu/libdbus-1.so.3"
    "/lib/x86_64-linux-gnu/libatspi.so.0"
    "/lib/x86_64-linux-gnu/libz.so.1"
    "/lib/x86_64-linux-gnu/libexpat.so.1"
    "/lib/x86_64-linux-gnu/libselinux.so.1"
    "/lib/x86_64-linux-gnu/libmount.so.1"
    "/lib/x86_64-linux-gnu/libblkid.so.1"
    "/lib/x86_64-linux-gnu/libuuid.so.1"
    "/lib/x86_64-linux-gnu/libbsd.so.0"
    "/lib/x86_64-linux-gnu/libpcre.so.3"
)

for lib in "${COMMON_LIBS[@]}"; do
    if [ -f "${lib}" ]; then
        copy_lib "${lib}"
    fi
done

echo "Done. Libraries copied to ${TARGET_LIB_DIR}"
