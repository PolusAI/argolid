#!/bin/bash
# prebuild_linux.sh
# Runs inside the manylinux_2_28_x86_64 Docker container during the prebuild job.
# Populates:
#   /project/prereq_cache/local_install  — filepattern + pybind11 headers/libs
#   /project/ts_fc_cache/               — TensorStore FetchContent source + build
set -e

cd /project

# Install NASM (required by aom/dav1d inside TensorStore)
curl -L https://www.nasm.us/pub/nasm/releasebuilds/2.15.05/nasm-2.15.05.tar.bz2 \
     -o nasm.tar.bz2
tar -xjf nasm.tar.bz2
cd nasm-2.15.05 && ./configure && make -j"$(nproc)" && make install
cd /project

# Build zlib, libjpeg-turbo, libpng to /usr/local (needed for TENSORSTORE_USE_SYSTEM_* flags)
bash ci-utils/install_sys_deps_linux.sh

# Build filepattern + pybind11 → prereq_cache/local_install
# (--skip-sys-deps: sys deps already installed above)
bash ci-utils/install_prereq_linux.sh ./prereq_cache/local_install --skip-sys-deps

# Pre-build TensorStore using the minimal cmake project (no Python, no pybind11 needed).
# FETCHCONTENT_BASE_DIR stores TensorStore source + build state in ts_fc_cache/
# so wheel-build jobs can skip the expensive TensorStore download and reuse cmake state.
cmake -S /project/ci-utils/tensorstore_prebuild \
      -B /tmp/ts_cmake_prebuild \
      -DFETCHCONTENT_BASE_DIR=/project/ts_fc_cache \
      -DTENSORSTORE_USE_SYSTEM_JPEG=ON \
      -DTENSORSTORE_USE_SYSTEM_ZLIB=ON \
      -DTENSORSTORE_USE_SYSTEM_PNG=ON \
      -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/ts_cmake_prebuild -j"$(nproc)"
