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

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

DOCKER_ENV="-e VSOMEIP_OFFER_ONCE=1 -e VSOMEIP_CONFIGURATION=/app/config/docker-notify-service.json -e VSOMEIP_APPLICATION_NAME=service-sample" # -e VSOMEIP_CLIENTSIDELOGGING=1

# DOCKER_VOL="-v $SCRIPT_DIR/vsomeip/config:/app/config"

# DOCKER_PORTS="-p 224.225.226.233:32344:32344/udp -p 224.244.224.245:30490:30490/udp -p 32344:32344/udp" #-p 30509:30509/udp
DOCKER_PORTS="-p 32344:32344/udp -p 30490:30490/udp" #-p 30509:30509/udp

DOCKER_NET="someip"
if ! docker network inspect "$DOCKER_NET" &>/dev/null; then
	docker network create "$DOCKER_NET"
fi

echo
echo
echo "Optionally execute the following commands in the container to update json file with required IPs:"
echo '  jq --arg ip $(hostname -I) '\''.unicast=$ip'\'' "$VSOMEIP_CONFIGURATION" > "$VSOMEIP_CONFIGURATION.tmp" && mv "$VSOMEIP_CONFIGURATION.tmp" "$VSOMEIP_CONFIGURATION"'
#echo '  route add -net 224.0.0.0/4 dev eth0'
echo

## Allow args to override default entrypoint. e.g. $0 bash
# ENTRYPOINT="/app/bin/notify-sample --udp"
ENTRYPOINT="/app/bin/run-service.sh"
[ -n "$1" ] && ENTRYPOINT="$*"

set -x
docker run --rm -it $DOCKER_VOL $DOCKER_ENV --network "$DOCKER_NET" $DOCKER_PORTS --name=someip-serv vsomeip $ENTRYPOINT
#--cap-add NET_ADMIN
