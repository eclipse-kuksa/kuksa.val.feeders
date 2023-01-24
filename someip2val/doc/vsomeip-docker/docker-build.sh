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

[ "$1" = "-v" ] && VERBOSE="--progress=plain" && shift

DOCKER_BUILDKIT=1 docker buildx build --platform linux/amd64 -f Dockerfile -t vsomeip . --load $VERBOSE $*
[ $? -eq 0 ] || exit 1

echo "Built Docker image:"
docker image ls | grep vsomeip

echo
echo "To start someip service container:"
echo "  ./docker-run-service.sh"
echo
echo "To start someip client container:"
echo "  ./docker-run-client.sh"
echo
