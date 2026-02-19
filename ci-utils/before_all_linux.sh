#!/bin/bash
# before_all_linux.sh
# Replaces the inline CIBW_BEFORE_ALL_LINUX command.
# Runs inside the manylinux container during the wheel-build job.
# Expects:
#   /project/prereq_cache/local_install  — populated by prebuild job (may be absent on cache miss)
set -e

cd /project

# Always install NASM (required by TensorStore's aom/dav1d deps)
curl -L https://www.nasm.us/pub/nasm/releasebuilds/2.15.05/nasm-2.15.05.tar.bz2 \
     -o nasm.tar.bz2
tar -xjf nasm.tar.bz2
cd nasm-2.15.05 && ./configure && make -j"$(nproc)" && make install
cd /project

mkdir -p /tmp/argolid_bld

if [ -d "/project/prereq_cache/local_install" ]; then
    echo "==> Prereq cache hit: restoring filepattern + pybind11"
    cp -r /project/prereq_cache/local_install /tmp/argolid_bld/local_install
    # zlib/jpeg/png cannot be cached from /usr/local (container-internal path),
    # so rebuild them quickly from source (~1 min total).
    bash ci-utils/install_sys_deps_linux.sh
else
    echo "==> Prereq cache miss: building filepattern + pybind11 + sys deps from scratch"
    bash ci-utils/install_prereq_linux.sh
    cp -r /project/local_install /tmp/argolid_bld/local_install
fi
