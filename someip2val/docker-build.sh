#!/bin/sh

[ "$1" = "-v" ] && VERBOSE="--progress=plain" && shift

DOCKER_BUILDKIT=1 docker buildx build --platform linux/amd64 -f Dockerfile -t vsomeip . --load $VERBOSE $* && \
[ $? -eq 0 ] || exit 1

echo "Built Docker image:"
docker image ls | grep vsomeip

echo "To start someip service container:"
echo "  ./docker-run-service.sh"
echo
echo "To start someip client container:"
echo "  ./docker-run-client.sh"
echo
