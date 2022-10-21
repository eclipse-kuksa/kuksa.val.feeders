- [SOME/IP integration in Docker containers](#someip-integration-in-docker-containers)
- [SOME/IP to Kuksa.VAL Feeder](#someip-to-kuksaval-feeder)
  - [Overview](#overview)
  - [Setup Development environment](#setup-development-environment)
    - [Prerequisites](#prerequisites)
    - [Building someip2val](#building-someip2val)
  - [Configuration](#configuration)
    - [vsomeip specific Configuration](#vsomeip-specific-configuration)
      - [Environment variables:](#environment-variables)
      - [Wiper configuration files:](#wiper-configuration-files)
      - [Config file modifications:](#config-file-modifications)
  - [Runing someip example and someip2val feeder](#runing-someip-example-and-someip2val-feeder)
    - [Local mode (single host)](#local-mode-single-host)
    - [UDP mode (2 hosts)](#udp-mode-2-hosts)
  - [Extending someip2val feeder](#extending-someip2val-feeder)

# SOME/IP integration in Docker containers

Example vsomeip dockerisation is described in details [here](docker/README.md)

# SOME/IP to Kuksa.VAL Feeder

## Overview

SOME/IP feeder is [vsomeip app](https://github.com/COVESA/vsomeip/), that subscribes for specific SOME/IP Events, parses its payload and feeds some of the data to KUKSA.VAL Databroker.

- [src/someip_feeder](./src/someip_feeder/) is the main SOME/IP to KUKSA.VAL Databroker adapter.
- [src/lib/broker_feeder](./src/lib/broker_feeder/) is provinding KUKSA.VAL Databroker integration.
- [src/lib/someip_client](./src/lib/someip_client/) is provinding generic SOME/IP Client implementation (generic implementation, does not depend on wiper).
- [src/lib/wiper_poc](./src/lib/wiper_poc/) is provinding wiper specific implementation (someip config, serialization, deserialization of events and data structures)

- [examples/wiper_service/wiper_server.cc](./examples/wiper_service/wiper_server.cc): an example SOME/IP Wiper Service for sending some serialized example Wiper events.
- [examples/wiper_service/wiper_server.cc](./examples/wiper_service/wiper_server.cc): an example SOME/IP Wiper Client for subscribing and parsing Wiper event payload. (no databroker integration)
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
1. Install and start KUKSA Databroker
    ``` bash
    # if running on x86_64 host:
    wget https://github.com/eclipse/kuksa.val/releases/download/databroker-v0.17.0/databroker_x86_64.tar.gz
    # if running on arm 64 host (rpi)
    wget https://github.com/eclipse/kuksa.val/releases/download/databroker-v0.17.0/databroker_aarch64.tar.gz
    # extract needed binaries
    tar xzvf databroker_x86_64.tar.gz --strip-components 3
    # start databroker
    ./databroker
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

#### Environment variables:
- `VSOMEIP_CONFIGURATION` : path to vsomeip config json file
- `VSOMEIP_APPLICATION_NAME`: vsomeip application name, must be consistent with json config file `.applications[].name`

#### Wiper configuration files:
- Wiper Service Config: [config/someip_wiper_service.json](./config/someip_wiper_service.json)
- Wiper Client / someip2val Config: [config/someip_wiper_client.json](./config/someip_wiper_client.json)
- Wiper Client / someip2val (Proxy) [config/someip_wiper_client-proxy.json](./config/someip_wiper_client-proxy.json)

**NOTE**: Proxy config should be used if Wiper Service is running on the same host with someip2val feeder. It has wiper service app as router, and client is proxy to local wiper service.

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