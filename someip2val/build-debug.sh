#!/bin/bash
#********************************************************************************
# Copyright (c) 2022 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License 2.0 which is available at
# http://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
#*******************************************************************************/

# Specify:
#   first argument: TARGET_ARCH = "x86_64", "rpi" or "aarch64"; default: "x86_64". "rpi" is used for compiling on raspberry pi
#   second argument: TARGET_ARCH = "<string>; default: "$SCRIPT_DIR/target/$TARGET_ARCH/Debug"

set -ex

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

TARGET_ARCH="$1"
[ -z "$TARGET_ARCH" ] && TARGET_ARCH="x86_64"

BUILD_DIR="$2"
[ -z "$BUILD_DIR" ] && BUILD_DIR="$SCRIPT_DIR"/target/"$TARGET_ARCH"/debug

cmake -E make_directory "$BUILD_DIR"

# install last known good boost version before conan v2 mess...
### experimental stuff
export CONAN_REVISIONS_ENABLED=1

echo "########## Conan Info #########"
conan --version
conan info .
echo "###############################"

# build with dependencies of build_type Debug
conan install -if="$BUILD_DIR" --build=missing --profile:build=default --profile:host="${SCRIPT_DIR}/toolchains/target_${TARGET_ARCH}_Release" "$SCRIPT_DIR"
cd "$BUILD_DIR" || exit
# shellcheck disable=SC1091
source activate.sh # Set environment variables for cross build

if [ "$VERBOSE" = "1" ]; then
	VERBOSE_OPT="-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON -DCONAN_CMAKE_SILENT_OUTPUT=OFF -DFETCHCONTENT_QUIET=OFF"
	VERBOSE_OPT="-LAH --debug-trace --debug-output $VERBOSE_OPT"
else
	VERBOSE_OPT="-DCONAN_CMAKE_SILENT_OUTPUT=ON"
fi
cmake $VERBOSE_OPT "$SCRIPT_DIR" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX="./install"

sleep 1
cmake --build . -j "$(nproc)"
cmake --install .

DIST="$SCRIPT_DIR/someip2val_${TARGET_ARCH}_debug.tar.gz"
cd "$BUILD_DIR/install" || exit 1
tar czvf "$DIST" bin/ lib/libvsomeip*.so*

echo
echo "### Created dist: $DIST"
echo