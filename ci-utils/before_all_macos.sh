#!/bin/bash
# before_all_macos.sh
# Replaces the inline CIBW_BEFORE_ALL_MACOS commands for wheel-build jobs.
# brew uninstalls (openssl for intel, jpeg-turbo for arm64) should be done
# by the caller (CIBW_BEFORE_ALL inline) BEFORE invoking this script.
# Expects:
#   prereq_cache/local_install  — populated by prebuild job (may be absent on cache miss)
set -e

mkdir -p /tmp/argolid_bld

if [ -d "prereq_cache/local_install" ]; then
    echo "==> Prereq cache hit: restoring filepattern + pybind11"
    cp -r prereq_cache/local_install /tmp/argolid_bld/local_install
    # Rebuild sys deps (zlib/jpeg/png) to /usr/local — these are fast and
    # cannot be cached across jobs since they live outside the workspace.
    bash ci-utils/install_sys_deps_linux.sh
else
    echo "==> Prereq cache miss: building filepattern + pybind11 + sys deps from scratch"
    bash ci-utils/install_prereq_linux.sh
    cp -r local_install /tmp/argolid_bld/local_install
fi
