#!/bin/bash
# install_sys_deps_linux.sh
# Builds and installs zlib, libjpeg-turbo, and libpng to /usr/local.
# Works on Linux (manylinux) and macOS.
# These are required by TensorStore when built with TENSORSTORE_USE_SYSTEM_* flags.
set -e

JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
BUILD_TMPDIR="$(mktemp -d)"

# zlib
curl -L https://github.com/madler/zlib/releases/download/v1.3.1/zlib131.zip -o "${BUILD_TMPDIR}/zlib131.zip"
unzip -q "${BUILD_TMPDIR}/zlib131.zip" -d "${BUILD_TMPDIR}"
mkdir -p "${BUILD_TMPDIR}/zlib-1.3.1/build_man"
cmake -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_INSTALL_PREFIX=/usr/local \
      -S "${BUILD_TMPDIR}/zlib-1.3.1" -B "${BUILD_TMPDIR}/zlib-1.3.1/build_man"
cmake --build "${BUILD_TMPDIR}/zlib-1.3.1/build_man" -j"${JOBS}"
cmake --build "${BUILD_TMPDIR}/zlib-1.3.1/build_man" --target install

# libjpeg-turbo
curl -L https://github.com/libjpeg-turbo/libjpeg-turbo/archive/refs/tags/3.1.0.zip \
     -o "${BUILD_TMPDIR}/3.1.0.zip"
unzip -q "${BUILD_TMPDIR}/3.1.0.zip" -d "${BUILD_TMPDIR}"
mkdir -p "${BUILD_TMPDIR}/libjpeg-turbo-3.1.0/build_man"
cmake -DCMAKE_INSTALL_PREFIX=/usr/local -DENABLE_STATIC=FALSE \
      -DCMAKE_BUILD_TYPE=Release \
      -S "${BUILD_TMPDIR}/libjpeg-turbo-3.1.0" \
      -B "${BUILD_TMPDIR}/libjpeg-turbo-3.1.0/build_man"
if [[ "$OSTYPE" == "darwin"* ]]; then
  sudo cmake --build "${BUILD_TMPDIR}/libjpeg-turbo-3.1.0/build_man" \
             --target install -j"${JOBS}"
else
  cmake --build "${BUILD_TMPDIR}/libjpeg-turbo-3.1.0/build_man" \
        --target install -j"${JOBS}"
fi

# libpng
curl -L https://github.com/glennrp/libpng/archive/refs/tags/v1.6.53.zip \
     -o "${BUILD_TMPDIR}/v1.6.53.zip"
unzip -q "${BUILD_TMPDIR}/v1.6.53.zip" -d "${BUILD_TMPDIR}"
mkdir -p "${BUILD_TMPDIR}/libpng-1.6.53/build_man"
cmake -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_INSTALL_PREFIX=/usr/local \
      -S "${BUILD_TMPDIR}/libpng-1.6.53" \
      -B "${BUILD_TMPDIR}/libpng-1.6.53/build_man"
cmake --build "${BUILD_TMPDIR}/libpng-1.6.53/build_man" \
      --target install -j"${JOBS}"

rm -rf "${BUILD_TMPDIR}"
