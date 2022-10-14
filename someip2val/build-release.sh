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
#   second argument: TARGET_ARCH = "<string>; default: "$SCRIPT_DIR/target/$TARGET_ARCH/release"

# shellcheck disable=SC1091
# shellcheck disable=SC2086
# shellcheck disable=SC2046
# shellcheck disable=SC2230

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

TARGET_ARCH="$1"
[ -z "$TARGET_ARCH" ] && TARGET_ARCH="x86_64"

BUILD_DIR="$2"
[ -z "$BUILD_DIR" ] && BUILD_DIR="$SCRIPT_DIR/target/$TARGET_ARCH/release"

[ "$VERBOSE" = "1" ] && set -x

set -e
cmake -E make_directory "$BUILD_DIR"
conan install -if="$BUILD_DIR" --build=missing --profile:build=default --profile:host="${SCRIPT_DIR}/toolchains/target_${TARGET_ARCH}_Release" "$SCRIPT_DIR"

cd "$BUILD_DIR"

source ./activate.sh # Set environment variables for cross build

#CMAKE_CXX_FLAGS_RELEASE="${CMAKE_CXX_FLAGS_RELEASE} -s"
if [ "$VERBOSE" = "1" ]; then
	VERBOSE_OPT="-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON -DCONAN_CMAKE_SILENT_OUTPUT=OFF -DFETCHCONTENT_QUIET=OFF"
	VERBOSE_OPT="-LAH --debug-trace --debug-output $VERBOSE_OPT"
else
	VERBOSE_OPT="-DCONAN_CMAKE_SILENT_OUTPUT=ON"
fi
cmake $VERBOSE_OPT -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="./install" "$SCRIPT_DIR"

cmake --build . -j $(nproc)
cmake --install .
set +e +x

# Ensure release is sripped
if [ "$TARGET_ARCH" = "aarch64" ]; then
	STRIP="$(which aarch64-linux-gnu-strip)"
else
	STRIP="strip"
fi

echo
echo "### Check for stripped binaries"
BINARIES="./install/bin/someip_feeder"

file $BINARIES
if [ -n "$STRIP" ]; then
	echo "### Stripping binaries in: $(pwd)"
	$STRIP -s --strip-unneeded $BINARIES
	file $BINARIES
	echo
fi
