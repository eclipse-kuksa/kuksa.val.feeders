#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

DOCKER_ENV="-e VSOMEIP_CONFIGURATION=/app/config/docker-notify-client.json -e VSOMEIP_APPLICATION_NAME=client-sample" # -e VSOMEIP_CLIENTSIDELOGGING=1

# DOCKER_VOL="-v $SCRIPT_DIR/vsomeip/config:/app/config"

#DOCKER_PORTS="-p 224.225.226.233:32344:32344/udp -p 224.244.224.245:30490:30490/udp"

DOCKER_NET="someip"
if ! docker network inspect "$DOCKER_NET" &>/dev/null; then
	docker network create "$DOCKER_NET"
fi

echo
#echo "route add -net 224.0.0.0/4 dev eth0"
echo "Optionally execute the following commands in the container to update json file with required IPs:"
echo '  SERV_IP=$(ping -4 -n -q -w 1 someip-serv | grep PING | cut -d " " -f 3 | tr -d "()")'
echo '  jq --arg ip $(hostname -I) --arg serv $SERV_IP '\''.unicast=$ip | .services[0].unicast=$serv'\'' "$VSOMEIP_CONFIGURATION" > "$VSOMEIP_CONFIGURATION.tmp" && mv "$VSOMEIP_CONFIGURATION.tmp" "$VSOMEIP_CONFIGURATION"'
echo

## Allow args to override default entrypoint. e.g. $0 bash
# ENTRYPOINT="/app/bin/subscribe-sample --udp"
ENTRYPOINT="/app/bin/run-client.sh"
[ -n "$1" ] && ENTRYPOINT="$*"

set -x
docker run --rm -it $DOCKER_VOL $DOCKER_ENV --network "$DOCKER_NET" $DOCKER_PORTS --name=someip-cli vsomeip $ENTRYPOINT
# --cap-add NET_ADMIN
