# SOME/IP integration in Docker containers

SOME/IP provides service method (e.g. get/set) or event notifications.
For event notifications it is mandatory to have Service Discovery enabled on both client and the service (they should join the same multicast ip:port).
For service invocation it is also possible to disable Service Discovery, but both service and client definitions must be specified in both server and client config json files.

## File Structure

- [vsomeip](https://github.com/COVESA/vsomeip) is cloned in Dockerfile.
- `./vsomeip/examples`: contains changes to vsomeip examples copied over vsomeip files in Dockerfile.
- `./vsomeip/config`: has docker specific configuration files for notify-sample and subscribe-sample examples.
- `./docker-build.sh`: Useful for local building of the image (you may need to instal missing tools manually).
- `./docker-run-service.sh`: Script for running service (pub) container from vsomeip image.
- `./docker-run-client.sh`: Script for running client (sub) container from vsomeip image.

## SOME/IP Configuration

It is possible to connect client and server in local mode (using unix sockets in /tmp/vsomeip*), but that scenario requires --network host and sharing host /tmp directory in the container.

For remote endpoints a docker network can be used, with the following configuration considerations:

### Docker SOMEIP Service configuration

`./vsomip/config/docker-notify-service.json` - configuration for notify-sample example.

Path must be set in `VSOMEIP_CONFIGURATION`, as well as `VSOMEIP_APPLICATION_NAME="service-sample"`

- service unicast address needs to be set with the container's IP address inside docker network. e.g.

    ```bash
    $ hostname -I
    172.18.0.5
    ```

  should be set in someip config:

    ```json
    {
        "unicast": "172.18.0.5"
    }
    ````

- service multicast for events `224.225.226.233:32344` needs to be published in docker, e.g.

    ```bash
    docker run -p 224.225.226.233:32344:32344/udp
    ```

    and must be consistent with the config json:

    ```json
    {
        "service": "0x1234",
        "instance": "0x5678",
        "unreliable": "30509",
        "multicast": {
            "address": "224.225.226.233",
            "port": "32344"
        }
    }
    ```

- service discovery multicast `224.244.224.245:30490` needs to be published in docker as well, e.g.

    ```bash
    docker run -p 224.244.224.245:30490:30490/udp`
    ```

    and must be consistent with the config json:

    ```json
    "routing": "service-sample",
    "service-discovery": {
        "enable": "true",
        "multicast": "224.244.224.245",
        "port": "30490",
        "protocol": "udp"
   }
   ```

### Docker SOMEIP client configuration

`./vsomip/config/docker-notify-client.json` - configuration for subscribe-sample example.

Path must be set in `VSOMEIP_CONFIGURATION`, as well as `VSOMEIP_APPLICATION_NAME="client-sample"`

- client unicast address needs to be set with the container's IP address inside docker network. e.g.

    ```bash
    $ hostname -I
    172.18.0.6
    ```

    should be set in someip config:

    ```json
    "unicast": "172.18.0.6"
    ````

- service-discovery multicast `224.244.224.245:30490` needs to be consistent with the service config:

    ```json
    "routing": "client-sample",
    "service-discovery": {
        "enable": "true",
        "multicast": "224.244.224.245",
        "port": "30490",
        "protocol": "udp"
    }
    ```

## Testing SOME/IP communication

1. Build local vsomeip image with: `./docker-build.sh`
1. Run notify service container: `./docker-run-service.sh`
1. Check service output (multicast ip should be published, unicast address should be correct for the container):

    ```text
    [info] Application(service-sample, 1277) is initialized (11, 100).
    [info] REGISTER EVENT(1277): [1234.5678.8778:is_provider=true]
    ...
    [debug] Joining to multicast group 224.244.224.245 from 172.18.0.2
    ...
    Setting event (Length=1).
    Setting event (Length=2).
    Setting event (Length=3).
    Setting event (Length=4).
    ```

1. Run notify client container: `./docker-run-client.sh`
1. Check service logs for these lines:

    ```text
    [info] Application(client-sample, 1344) is initialized (11, 100).
    Client settings [protocol=UDP]
     [info] REGISTER EVENT(1344): [1234.5678.8778:is_provider=false]
     [info] SUBSCRIBE(1344): [1234.5678.4465:ffff:0]
    ...
     [info] REQUEST(1344): [1234.5678:255.4294967295]
     [info] udp_server_endpoint_impl: SO_RCVBUF is: 212992
    [debug] Joining to multicast group 224.244.224.245 from 172.18.0.3
    ...
    Service [1234.5678] is available.
    Received a notification for Event [1234.5678.8778] to Client/Session [0000/019a] = (1) 00
    Received a notification for Event [1234.5678.8778] to Client/Session [0000/019b] = (2) 00 01
    Received a notification for Event [1234.5678.8778] to Client/Session [0000/019c] = (3) 00 01 02
    Received a notification for Event [1234.5678.8778] to Client/Session [0000/019d] = (4) 00 01 02 03
    ```

    Notification payload is byte[] received from notify service container. So far it is not forwarded to Kuksa.VAL


**NOTE:**
In case some of those messages are missing you can try running bash entrypoint and manually starting someip apps, e.g.:

- `./docker-run-service.sh bash`
- `./docker-run-client.sh bash`
