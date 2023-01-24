# SOME/IP to KUKSA.val Provider

- [SOME/IP to KUKSA.val Provider](#someip-to-kuksaval-provider)
  - [Overview](#overview)
  - [someip2val Components](#someip2val-components)
  - [Setup Development environment](#setup-development-environment)
    - [Prerequisites](#prerequisites)
    - [Building someip2val](#building-someip2val)
  - [Configuration](#configuration)
    - [vsomeip specific Configuration](#vsomeip-specific-configuration)
      - [Environment variables for vsomeip](#environment-variables-for-vsomeip)
      - [Wiper configuration files](#wiper-configuration-files)
      - [Config file modifications](#config-file-modifications)
  - [Containerization](#containerization)
    - [Running SOME/IP containers with host networking on a single host](#running-someip-containers-with-host-networking-on-a-single-host)
    - [Running SOME/IP containers in Docker network](#running-someip-containers-in-docker-network)
  - [Runing SOME/IP example service and someip2val feeder](#runing-someip-example-service-and-someip2val-feeder)
    - [Local mode (single host)](#local-mode-single-host)
    - [UDP mode (2 hosts)](#udp-mode-2-hosts)
    - [Containerized setup](#containerized-setup)
  - [Extending someip2val feeder](#extending-someip2val-feeder)

----------------

## Overview

SOME/IP to KUKSA.val (**someip2val**) provides integration for an example SOME/IP "Wiper" ECU to [KUKSA.val databroker](https://github.com/eclipse/kuksa.val/tree/master/kuksa_databroker).

- [SOME/IP](http://some-ip.com) is an automotive communication protocol which supports remote procedure calls, event notifications, service discovery.
  SOME/IP messages are sent as TCP/UDP unicast/multicast packets, but it is also possible to use local (Unix) endpoints.

- **someip2val** is a [vsomeip](https://github.com/COVESA/vsomeip/) application, that subscribes for specific "Wiper" SOME/IP Events, parses the payload and feeds [VSS](https://github.com/COVESA/vehicle_signal_specification) datapoints to KUKSA.val databroker.

- **someip2val** also provides an example "Wiper" SOME/IP request/response handling for setting wiper parameters.

- An example "Wiper" SOME/IP service is also provided to allow testing **someip2val**.

- For details on SOME/IP protocol and vsomeip architecture, check [vsomeip wiki](https://github.com/COVESA/vsomeip/wiki/vsomeip-in-10-minutes#intro).


It listens for specific SOME/IP events and feeds incoming [VSS](https://github.com/COVESA/vehicle_signal_specification) datapoints to **KUKSA.val databroker**. It also subscribes for actuator targets from VSS tree, so if all required datapoints are set, SOME/IP request is sent to the ECU/Simulator.

[![License](https://img.shields.io/badge/License-Apache%202.0-green.svg)](https://opensource.org/licenses/Apache-2.0)

## someip2val Components

- [src/someip_feeder/](./src/someip_feeder/) is the main SOME/IP to KUKSA.val Databroker adapter.
- [src/lib/broker_feeder/](./src/lib/broker_feeder/) is provinding KUKSA.val Databroker integration.
- [src/lib/someip_client/](./src/lib/someip_client/) is provinding generic SOME/IP Client implementation (generic implementation, does not depend on wiper).
- [src/lib/wiper_poc/](./src/lib/wiper_poc/) is provinding wiper specific implementation (someip config, serialization, deserialization of events and data structures).\
**NOTE**: Check [wiper_poc.h](src/lib/wiper_poc/wiper_poc.h) for SOME/IP Event definitions (`struct t_Event`), and SOME/IP Request (`struct t_WiperRequest`)
- [examples/wiper_service/wiper_server.cc](./examples/wiper_service/wiper_server.cc): an example SOME/IP Wiper Service for sending some serialized example Wiper events.
- [examples/wiper_service/wiper_client.cc](./examples/wiper_service/wiper_client.cc): an example SOME/IP Wiper Client for subscribing and parsing Wiper event payload and example Request/Response client for Wiper VSS service.
- [examples/wiper_service/wiper_sim.cc](./examples/wiper_service/wiper_sim.cc): an example simulation of a Wiper service.
- [patches/](./patches/): Contains vsomeip patches (master branch), that have not been pushed to upstream yet.
- [doc/](./doc/): Contains script for generating vsomeip User Guide, check [vsomeip.md](./doc/vsomeip.md)

## Setup Development environment

### Prerequisites

You need a Linux distribution to work with vsomeip (packages below are for Ubuntu):

1. Install cmake and build requirements
    ```sh
    sudo apt-get install -y cmake g++ build-essential g++-aarch64-linux-gnu binutils-aarch64-linux-gnu jq
    ```
1. Install and configure conan (if needed)
    ```sh
    sudo apt-get install -y python3 python3-pip
    pip3 install conan
    ```
    **NOTE:** Sometimes latest conan recipe revisions are broken, but the local build succeeds using cached older revision. If build fails on CI local conan cache could be cleared to reproduce the error. Also latest recipes may require newer conan version.
    ```sh
    rm -rf ~/.conan/data
    pip3 install -U conan
    ```
    Last known working revisions are hardcoded in [conanfile.txt](./conanfile.txt) [requires].
1. Install [VS Code](https://code.visualstudio.com/download). To setup proper conan environment in vs code, launch vscode using:
    ```sh
    ./vscode-conan.sh
    ```
1. Start KUKSA Databroker container (version with collector interface supporting `SubscribeActuatorTargets`)
    ```sh
    docker run -d --name databroker -p 55555:55555/tcp ghcr.io/boschglobal/kuksa.val/databroker:0.0.2
    ```
1. Install kuksa-client package/docker image for sending multiple actuator values at the same time:
    ```sh
    pip install -U kuksa-client
    ```
    or follow the instructions from [kuksa-client](https://github.com/eclipse/kuksa.val/blob/master/kuksa-client/README.md)
  1. vsomeip User Guide can be build as descibed in [doc/vsomeip.md](./doc/vsomeip.md)

### Building someip2val

There are scripts for building release and debug versions of someip2val feeder, supporting `x86_64`, `aarch64` or `rpi` architectures:

```sh
cd someip2val
./build-release.sh <arch>
```
**NOTE:** Use `rpi` when building on a Raspberry Pi.
Scripts generate `someip2val-<debug|release>-<arch>.tar` archives.

There is also a script for exporting OCI container images (or import them locally for testing):
```
./docker-build.sh [OPTIONS] TARGETS

Standalone build helper for someip-feeder container.

OPTIONS:
  -l, --local      local docker import (does not export tar)
  -v, --verbose    enable plain docker output and disable cache
      --help       show help

TARGETS:
  x86_64|amd64, aarch64|amd64    Target arch to build for, if not set - defaults to multiarch
```
**NOTE:** This script can't handle multi-arch images!

## Configuration

vsomeip requires a combination of json config file + environment variables.

### vsomeip specific Configuration

vsomeip library uses a combination of environment variables and config json files that must be set correctly or binaries won't work.
You can test vsomeip services in a "local" mode (running on a single Linux host, using Unix sockets for communication) or in "normal" mode, where 2 different hosts are required (e.g. wiper service running on the 1st host and someip2val feeder running on the 2nd host).

**NOTE:** Multicast config (`service-discovery`) for both services must be matching and multicast packages between the hosts must be enabled, also unicast messages between hosts must be possible (both hosts in the same network).

#### Environment variables for vsomeip
- `VSOMEIP_CONFIGURATION`: path to vsomeip config json file.
- `VSOMEIP_APPLICATION_NAME`: vsomeip application name, must be consistent with json config file `.applications[].name`

**NOTE**: Those variables are already set (and validated) in provided `./bin/setup-*.sh` scripts.

#### Wiper configuration files
- Wiper Service Config: [config/someip_wiper_service.json](./config/someip_wiper_service.json)
- Wiper Client Config: [config/someip_wiper_client.json](./config/someip_wiper_client.json)
- Wiper Client Config (Proxy) [config/someip_wiper_client-proxy.json](./config/someip_wiper_client-proxy.json)
- Someip Feeder Config: [config/someip_feeder.json](./config/someip_feeder.json)
- Someip Feeder Config (Proxy): [config/someip_feeder-proxy.json](./config/someip_feeder-proxy.json)

**NOTE**: With vsomeip it is not possible to have multiple routing applications running on the same host, so in Proxy setup, Wiper service is configured as routing app and Proxy clients are configured to route through Wiper Service.
In case two hosts (VMs) are available, Proxy configs are not needed, then one host should run the service and the other - client config.

#### Config file modifications
In order to use non-proxy mode on 2 network hosts, you have to modify the `.unicast` address in vsomeip config file, unfortunately it does not support hostnames, so there are some helper scripts for setting up the environment and replacing hostnames with `jq`
- Environment setup for Wiper Service: [./bin/setup-wiper-service.sh](./bin/setup-wiper-service.sh)
- Environment setup for Wiper Client: [./bin/setup-someip2val.sh](./bin/setup-someip2val.sh)
- Environment setup for Wiper Client (Proxy): [./bin/setup-someip2val-proxy.sh](./bin/setup-someip2val-proxy.sh)

## Containerization

vsomeip has an additional limitation regarding containers - it needs access to `/tmp`, where it creates its main routing application unux socket `/tmp/vsomeip-0`. This means that if you want to use host networking (allowing access to external SOME/IP hosts), both containers must have mounted `/tmp` to access the master routing vsomeip app.

### Running SOME/IP containers with host networking on a single host


```sh
# NOTE: if you want to access someip routing app on host, map /tmp, othwise it is ok to just have a shared dir for containers
[ -d /tmp/vsomeip ] || mkdir -p /tmp/vsomeip

# start databroker and expose its 55555 port to host
docker run -d --name databroker -p 55555:55555/tcp ghcr.io/boschglobal/kuksa.val/databroker:0.0.2

# start wiper service (as host someip routing app) with alternative entry point
docker run -d --name wiper-service --network host -v /tmp/vsomeip:/tmp ghcr.io/eclipse/kuksa.val.feeders/someip-feeder:latest \
  bash -c '{ source ./setup-wiper-service.sh; ./wiper_service; }'

# start someip2val feeder (proxy mode) with alternative entry point
docker run -it --rm --name someip2val --network host -v /tmp/vsomeip:/tmp ghcr.io/eclipse/kuksa.val.feeders/someip-feeder:latest \
  bash -c '{ source ./setup-someip2val-proxy.sh; ./someip_feeder; }'
# Expected output - parsed wiper events
### |WiperEvent| Pos: 15.0000, DC: 0.00, Wiping:0, CycEnd:0 PosReach:1, ...
```

### Running SOME/IP containers in Docker network

Alternativle approach is to use dedicated docker network for the wiper service and someip2val feeder containers, so they
can discover each other, but without access to external SOME/IP services.

```sh
# ensure someip docker network is created
docker network ls | grep -q someip || docker network create someip

# launch databroker in someip network
docker run -d --network someip --name someip_databroker ghcr.io/boschglobal/kuksa.val/databroker:0.0.2

# launch wiper service in someip network
docker run -d --name someip_wiper-service --network someip ghcr.io/eclipse/kuksa.val.feeders/someip-feeder:latest \
  bash -c '{ source ./setup-wiper-service.sh; ./wiper_service; }'

# launch someip2val feeder, note BROKER_ADDDR should match container name of databroker image in the same network
docker run -it --rm --name someip_someip2val --network someip -e BROKER_ADDR=someip_databroker:55555 ghcr.io/eclipse/kuksa.val.feeders/someip-feeder:latest

```

For testing, use kuksa-client (locally installed or containerized):
```sh
docker run -it --rm -e GRPC_ENABLE_FORK_SUPPORT=true --net=host kuksa-client:latest --ip 127.0.0.1 --port 55555 --protocol grpc --insecure
```
Execute the following command to move wiper at position 100.0, freq 1:
```
setTargetValues Vehicle.Body.Windshield.Front.Wiping.System.Frequency=1 Vehicle.Body.Windshield.Front.Wiping.System.Mode=WIPE Vehicle.Body.Windshield.Front.Wiping.System.TargetPosition=100
```

You should see `|WiperEvent|` with position 100:
```
### |WiperEvent| Pos: 99.9392, DC:10.02, Wiping:1, CycEnd:0 PosReach:0, ...
### |WiperEvent| Pos:100.0780, DC:10.00, Wiping:1, CycEnd:1 PosReach:0, ...
### |WiperEvent| Pos:100.0929, DC:10.03, Wiping:1, CycEnd:1 PosReach:0, ...
### |WiperEvent| Pos:100.0000, DC: 9.95, Wiping:0, CycEnd:0 PosReach:1, ...
### |WiperEvent| Pos:100.0000, DC: 0.00, Wiping:0, CycEnd:0 PosReach:1, ...
```

## Runing SOME/IP example service and someip2val feeder

Setup scripts in `./bin` are meant to run from install directory, e.g.
after executing `./build-debug.sh`  it is: `target/x86_64/debug/install/bin`.

If running from another location, make sure your `LD_LIBRARY_PATH` includes vsomeip3 binaries.
### Local mode (single host)
In this mode only Unix sockets are used, wiper service is acting as a SOME/IP router app and someip2val feeder is a proxy.

- Launch wiper service from install directory:
```sh
. ./setup-wiper-service.sh
./wiper_service --cycle 300
```
- Launch someip2val feeder in proxy mode:
```sh
. ./setup-someip2val-proxy.sh
./someip_feeder
```
### UDP mode (2 hosts)
In this mode you need another host in your network to run the service.

- Launch wiper service from install directory on Host2:
```sh
. ./setup-wiper-service.sh
./wiper_service --cycle 300
```
- Launch someip2val feeder in default mode:
```sh
. ./setup-someip2val.sh
./someip_feeder
```

Make sure you have `jq` installed as it is rewriting config files to update unicast address.

### Containerized setup

It is advised to use host networking for SOME/IP containers, although it is also possible to use a dedicated docker network, but in that case external SOME/IP communication won't be possible.

1. Make sure KUKSA databroker container is runnning and note its host port (55555 by default)
2. To start someip2val feeder container execute:
```sh
docker run -it --rm --name someip-feeder -e BROKER_ADDR=0.0.0.0:55555 --network host ghcr.io/eclipse/kuksa.val.feeders/someip-feeder:latest
```
**NOTE:** Adjust `BROKER_ADDR` to match host:port of databroker
3. It is also possible to start a wiper service instance from the same image, replacing entrypoint with `sh`:
```sh
docker run -it --rm --name wiper-service --network host ghcr.io/eclipse/kuksa.val.feeders/someip-feeder:latest /bin/bash

```


## Extending someip2val feeder

Provided wiper example needs to be adjusted for another someip service events.

- `SomeIPClient` class provides generic event subscription and passes someip payload to a custom callback:
``` c++
typedef std::function <
  int (vsomeip::service_t service, vsomeip::instance_t instance, vsomeip::method_t event,
      const uint8_t *payload, size_t size)
> message_callback_t;
```
- `SomeIPConfig` vsomeip service/instance/event_group/event values also have to be changed (e.g. via environment variables, or in code)
- `SomeipFeederAdapter::on_someip_message()` : Example for someip payload callback, deserializing payload and feeding to Databroker
