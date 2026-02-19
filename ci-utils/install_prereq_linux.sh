#!/bin/bash
# Usage: bash install_prereq_linux.sh [$INSTALL_DIR] [--skip-sys-deps]
# Default $INSTALL_DIR = ./local_install
# --skip-sys-deps: skip building zlib, libjpeg-turbo, libpng (use when already installed)
#
SKIP_SYS_DEPS=false
for arg in "$@"; do
    case "$arg" in
        --skip-sys-deps) SKIP_SYS_DEPS=true ;;
    esac
done

if [ -z "$1" ] || [ "$1" = "--skip-sys-deps" ]
then
      echo "No path to the Argolid source location provided"
      echo "Creating local_install directory"
      LOCAL_INSTALL_DIR="local_install"
else
     LOCAL_INSTALL_DIR=$1
fi

mkdir -p $LOCAL_INSTALL_DIR
mkdir -p $LOCAL_INSTALL_DIR/include

curl -L https://github.com/PolusAI/filepattern/archive/refs/tags/v2.0.4.zip -o v2.0.4.zip 
unzip v2.0.4.zip
cd filepattern-2.0.4
mkdir build
cd build
cmake -Dfilepattern_SHARED_LIB=ON -DCMAKE_PREFIX_PATH=../../$LOCAL_INSTALL_DIR -DCMAKE_INSTALL_PREFIX=../../$LOCAL_INSTALL_DIR ../src/filepattern/cpp
make install -j4
cd ../../


curl -L https://github.com/pybind/pybind11/archive/refs/tags/v2.12.0.zip -o v2.12.0.zip
unzip v2.12.0.zip
cd pybind11-2.12.0
mkdir build_man
cd build_man
cmake -DCMAKE_INSTALL_PREFIX=../../$LOCAL_INSTALL_DIR/  -DPYBIND11_TEST=OFF ..
make install -j4
cd ../../


if [ "$SKIP_SYS_DEPS" != "true" ]; then
curl -L https://github.com/madler/zlib/releases/download/v1.3.1/zlib131.zip -o zlib131.zip
unzip zlib131.zip
cd zlib-1.3.1
mkdir build_man
cd build_man
cmake -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_INSTALL_PREFIX=/usr/local ..
cmake --build .
cmake --build . --target install
cd ../../

curl -L https://github.com/libjpeg-turbo/libjpeg-turbo/archive/refs/tags/3.1.0.zip -o 3.1.0.zip
unzip 3.1.0.zip
cd libjpeg-turbo-3.1.0
mkdir build_man
cd build_man
cmake -DCMAKE_INSTALL_PREFIX=/usr/local -DENABLE_STATIC=FALSE -DCMAKE_BUILD_TYPE=Release ..
if [[ "$OSTYPE" == "darwin"* ]]; then
  sudo make install -j4
else
  make install -j4
fi
cd ../../
make install -j4
cd ../../

curl -L  https://github.com/glennrp/libpng/archive/refs/tags/v1.6.53.zip -o v1.6.53.zip
unzip v1.6.53.zip
cd libpng-1.6.53
mkdir build_man
cd build_man
cmake -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_INSTALL_PREFIX=/usr/local ..
make install -j4
cd ../../
fi
