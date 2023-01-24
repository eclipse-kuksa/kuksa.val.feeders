#!/bin/sh
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

# important: match VSOMEIP_APPLICATION_NAME with VSOMEIP_CONFIGURATION config, must be equal!
export VSOMEIP_APPLICATION_NAME="client-sample"
export VSOMEIP_CONFIGURATION="/app/config/docker-notify-client.json"
# export VSOMEIP_CLIENTSIDELOGGING=""

# make sure unicast ip is set in config for this container
jq --arg ip "$(hostname -I | cut -d ' ' -f 1)" '.unicast=$ip' "$VSOMEIP_CONFIGURATION" > "$VSOMEIP_CONFIGURATION.tmp" && mv "$VSOMEIP_CONFIGURATION.tmp" "$VSOMEIP_CONFIGURATION"

echo "Running APP[$VSOMEIP_APPLICATION_NAME] with config $VSOMEIP_CONFIGURATION"
cat "$VSOMEIP_CONFIGURATION"

set -x
/app/bin/subscribe-sample --udp
