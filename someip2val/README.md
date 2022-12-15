- [SOME/IP integration in Docker containers](#someip-integration-in-docker-containers)
- [SOME/IP to Kuksa.VAL Feeder](#someip-to-kuksaval-feeder)
  - [Overview](#overview)
  - [Setup Development environment](#setup-development-environment)
    - [Prerequisites](#prerequisites)
    - [Building someip2val](#building-someip2val)
  - [Configuration](#configuration)
    - [vsomeip specific Configuration](#vsomeip-specific-configuration)
      - [Environment variables for vsomeip:](#environment-variables-for-vsomeip)
      - [Wiper configuration files:](#wiper-configuration-files)
      - [Config file modifications:](#config-file-modifications)
  - [Runing someip example and someip2val feeder](#runing-someip-example-and-someip2val-feeder)
    - [Local mode (single host)](#local-mode-single-host)
    - [UDP mode (2 hosts)](#udp-mode-2-hosts)
  - [Extending someip2val feeder](#extending-someip2val-feeder)

# SOME/IP integration in Docker containers

Running default vsomeip examples in containers is described in details [here](docker/README.md)

# SOME/IP to Kuksa.VAL Feeder

## Overview

SOME/IP feeder is [vsomeip app](https://github.com/COVESA/vsomeip/), that subscribes for specific SOME/IP Events, parses its payload and feeds some of the data to KUKSA.VAL Databroker.
It also provides an example SOME/IP request / response handling.

- [src/someip_feeder](./src/someip_feeder/) is the main SOME/IP to KUKSA.VAL Databroker adapter.
- [src/lib/broker_feeder](./src/lib/broker_feeder/) is provinding KUKSA.VAL Databroker integration.
- [src/lib/someip_client](./src/lib/someip_client/) is provinding generic SOME/IP Client implementation (generic implementation, does not depend on wiper).
- [src/lib/wiper_poc](./src/lib/wiper_poc/) is provinding wiper specific implementation (someip config, serialization, deserialization of events and data structures).

- [examples/wiper_service/wiper_server.cc](./examples/wiper_service/wiper_server.cc): an example SOME/IP Wiper Service for sending some serialized example Wiper events.
- [examples/wiper_service/wiper_server.cc](./examples/wiper_service/wiper_server.cc): an example SOME/IP Wiper Client for subscribing and parsing Wiper event payload and example Request/Response client for Wiper VSS service.
- [examples/wiper_service/wiper_sim.cc](./examples/wiper_service/wiper_sim.cc): an example simulation of a Wiper service.
- [patches](./patches): Contains vsomeip patches (master branch), that have not been pushed to upstream yet.

## Setup Development environment

### Prerequisites

1. Install cmake and build requirements
    ``` bash
    sudo apt-get install -y cmake g++ build-essential g++-aarch64-linux-gnu binutils-aarch64-linux-gnu jq
    ```
1. Install and configure conan (if needed)
    ``` bash
    sudo apt-get install -y python3 python3-pip
    pip3 install conan
    ```
1. Install [VS Code](https://code.visualstudio.com/download). To setup proper conan environment in vs code, launch vscode using:
    ``` bash
    ./vscode-conan.sh
    ```
1. Install and start KUKSA Databroker (version with collector interface supporting `SubscribeActuatorTargets`)
    ``` bash
    docker run --rm -it -p 55555:55555/tcp ghcr.io/boschglobal/kuksa.val/databroker:0.0.2
    ```

### Building someip2val

There are scripts for building release and debug versions of someip2val feeder, supporting `x86_64`, `aarch64` or `rpi` architectures:

``` bash
cd someip2val
./build-release.sh <arch>
```

*NOTE:* Use `rpi` when building on a Raspberry Pi.

Scripts generate `someip2val-<debug|release>-<arch>.tar` archives

## Configuration

vsomeip requires a combination of json config file + environment variables

### vsomeip specific Configuration

#### Environment variables for vsomeip:
- `VSOMEIP_CONFIGURATION` : path to vsomeip config json file
- `VSOMEIP_APPLICATION_NAME`: vsomeip application name, must be consistent with json config file `.applications[].name`
**NOTE**: Those variables are already set in provided `setup-*.sh` scripts.

#### Wiper configuration files:
- Wiper Service Config: [config/someip_wiper_service.json](./config/someip_wiper_service.json)
- Wiper Client Config: [config/someip_wiper_client.json](./config/someip_wiper_client.json)
- Wiper Client Config (Proxy) [config/someip_wiper_client-proxy.json](./config/someip_wiper_client-proxy.json)
- Someip Feeder Config: [config/someip_feeder.json](./config/someip_feeder.json)
- Someip Feeder Config (Proxy): [config/someip_feeder-proxy.json](./config/someip_feeder-proxy.json)

**NOTE**: With vsomeip it is not possible to have multiple routing applications running on the same host, so in Proxy setup, Wiper service is configured as routing app and Proxy clients are configured to route through Wiper Service.
In case two hosts (VMs) are available, Proxy configs are not needed, then one host should run the service and the other - client config.

#### Config file modifications:
In order to use non-proxy mode on 2 network hosts, you have to modify the `.unicast` address in vsomeip config file, unfortunately it does not support hostnames, so there are some helper scripts for setting up the environment and replacing hostnames with `jq`
- Environment setup for Wiper Service: [./bin/setup-wiper-service.sh](./bin/setup-wiper-service.sh)
- Environment setup for Wiper Client: [./bin/setup-someip2val.sh](./bin/setup-someip2val.sh)
- Environment setup for Wiper Client (Proxy): [./bin/setup-someip2val-proxy.sh](./bin/setup-someip2val-proxy.sh)


## Runing someip example and someip2val feeder

Setup scripts in `./bin` are meant to run from install directory, e.g.
after executing `./build-debug.sh`  it is: `target/x86_64/debug/install/bin`.

If running from another location, make sure your `LD_LIBRARY_PATH` includes vsomeip3 binaries.
### Local mode (single host)
In this mode only Unix sockets are used, wiper service is acting as a someip router app and someip2val feeder is a proxy.

- Launch wiper service from install directory:
``` bash
. ./setup-wiper-service.sh
./wiper_service --cycle 300
```
- Launch someip2val feeder in proxy mode:
``` bash
. ./setup-someip2val-proxy.sh
./someip_feeder
```
### UDP mode (2 hosts)
In this mode you need another host in your network to run the service.

- Launch wiper service from install directory on Host2:
``` bash
. ./setup-wiper-service.sh
./wiper_service --cycle 300
```
- Launch someip2val feeder in default mode:
``` bash
. ./setup-someip2val.sh
./someip_feeder
```

Make sure you have `jq` installed as it is rewriting config files to update unicast address.

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