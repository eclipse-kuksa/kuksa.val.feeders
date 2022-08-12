#!/bin/sh

IF=$1

[ -z "$IF" ] && IF="eth0"

route add -net 224.0.0.0/4 dev "$IF"
