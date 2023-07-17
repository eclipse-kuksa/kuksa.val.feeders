# GPS Feeder
consumes [gpsd](https://gpsd.gitlab.io/gpsd/) as datasource and pushes location to kuksa.val server.
The [`gpsd_feeder.ini`](./config/gpsd_feeder.ini) contains `kuksa.val` and `gpsd` configuration.

Before starting the gps feeder, you need to start `kuksa.val server/databroker` (for this have a look at [KUKSA.val server]( https://github.com/eclipse/kuksa.val/blob/master/kuksa-val-server/README.md) and [KUKSA.val databroker]( https://github.com/eclipse/kuksa.val/blob/master/kuksa_databroker/README.md)). You have to start an instance of `gpsd` by running:
```
gpsd -S <gpsd port> -N <gps device>
```

If you do not have a gps device, you can use your cellphone to forward gps data to `gpsd`. For example [gpsd-forward](https://github.com/tiagoshibata/Android-GPSd-Forwarder) is an open source android app. You can start gpsd with the following command to receive data from the app:

```
gpsd -N udp://0.0.0.0:29998
```
## Install dependencies and execution

```
usage: gpsd_feeder.py [-h] [--ip [IP]] [--port [PORT]] [--protocol [PROTOCOL]] [--insecure [INSECURE]] [--cacertificate [CACERTIFICATE]] [--tls-server-name [TLS_SERVER_NAME]] [--token [TOKEN]]
                      [--file [FILE]] [--gpsd_host [GPSD_HOST]] [--gpsd_port [GPSD_PORT]] [--interval [INTERVAL]]

options:
  -h, --help            show this help message and exit
  --ip [IP]             Specify the host where too look for KUKSA.val server/databroker; default: 127.0.0.1
  --port [PORT]         Specify the port where too look for KUKSA.val server/databroker; default: 8090
  --protocol [PROTOCOL]
                        If you want to connect to KUKSA.val server specify ws. If you want to connect to KUKSA.val databroker specify grpc; default: ws
  --insecure [INSECURE]
                        Specify if you want an insecure connection (i.e. without TLS)default: False
  --cacertificate [CACERTIFICATE]
                        Specify the path to your CA.pem; default: CA.pem. Needed when not using insecure mode
  --tls-server-name [TLS_SERVER_NAME]
                        TLS server name, may be needed if addressing a server by IP-name
  --token [TOKEN]       Specify the JWT token string or the path to your JWT token; default: authentication information not specified
  --file [FILE]         Specify the path to your config file; by default not defined
  --gpsd_host [GPSD_HOST]
                        Specify the host for gpsd to start on; default: 127.0.0.1
  --gpsd_port [GPSD_PORT]
                        Specify the port for gpsd to start on; default: 2948
  --interval [INTERVAL]
                        Specify the interval time for feeding gps data; default: 1
```

A template config file that can be used together with the `--file` option
exists in [config/gpsd_feeder.ini](config/gpsd_feeder.ini). Note that if `--file` is specified all other options
are ignored, instead the values in the config file or default values specified by kuksa-client will be used.

```
pip install -r requirements.txt
python gpsd_feeder.py
```

## Authorization

gpsd_feeder will try to authenticate itself towards the KUKSA.val Server/Databroker if a token is given.
Note that the KUKSA.val Databroker by default does not require authentication.

An example for authorizing against KUKSA.val Databroker using an example token is shown below.

```
python gpsd_feeder.py --protocol grpc --port 55555 --insecure true --token /home/user/kuksa.val/jwt/provide-all.token
```

## TLS

The KUKSA.val GPS Feeder supports using TLS connections. A TLS connection will be used unless `--insecure=True`
is specified. When using a TLS connection a path to the root certificate used by the Server/Databroker must be given.
The client validates the name of the server against the certificate provided by the Server/Databroker.
If addressing with a numeric IP-address and using grpc as protocol the "real" server name must be given using
`--tls-server-name`. For the [KUKSA.val example certificates](https://github.com/eclipse/kuksa.val/tree/master/kuksa_certificates)
the names `localhost`and `Server`can be used.

```
python gpsd_feeder.py --port 55555 --protocol grpc --ip 127.0.0.1 --cacertificate ~/kuksa.val/kuksa_certificates/CA.pem --tls-server-name Server
```

### Using docker
You can also use `docker` to execute the feeder platform independently.
To build a docker image:
```
docker build -t gpsd_feeder .
```

You can also download docker images from [our container registry](https://github.com/eclipse/kuksa.val.feeders/pkgs/container/kuksa.val.feeders%2Fgps).

To run:
```
docker run -it -p 29998:29998/udp -v $PWD/config:/config gpsd_feeder
```

The container contains an internal gpsd daemon and the exposed UDP port can be used to feed NMEA data e.g. with [gpsd-forward](https://github.com/tiagoshibata/Android-GPSd-Forwarder) from an Android phone. If you already have a configured GPSd, just modify the config file to point to it.

Keep in mind, that GPSd normally only listens on localhost/loopback interface. To connect it from another interface start gpsd with the `-D` option

## Test with gpsfake
You can also use [gpsfake](https://gpsd.gitlab.io/gpsd/gpsfake.html) to playback a gps logs in e.g. nmea format.
To install `gpsfake`, follow the command in this [link](https://command-not-found.com/gpsfake).
After installation, run the following command to simulate a gps device as datasource:

```
gpsfake -P 2947 simplelog_example.nmea
```

Note: You need to use the `gpsfake` with the same version like the installed `gpsd`.

There are several tools for generating nmea log files:
- [nmea-gps logger](https://www.npmjs.com/package/nmea-gps-logger)
- [nmeagen](https://nmeagen.org/)

### gpsfake troubleshouting
If you see a gpsfake error message similar to this one after the feeder connected:

```
gpsfake: log cycle of simplelog_example.nmea begins.
gpsd:ERROR: SER: device open of /dev/pts/8 failed: Permission denied - retrying read-only
gpsd:ERROR: SER: read-only device open of /dev/pts/8 failed: Permission denied
gpsd:ERROR: /dev/pts/8: device activation failed, freeing device.
```

This might be due to a an overly restrictive apparmor configuration. On Ubuuntu edit the file `/etc/apparmor.d/usr.sbin.gpsd`

search for a section looking like this

```
  # common serial paths to GPS devices
  /dev/tty{,S,USB,AMA,ACM}[0-9]*    rw,
  /sys/dev/char     r,
  /sys/dev/char/**  r,
```

And add a line for pts device so that it looks like

```
  # common serial paths to GPS devices
  /dev/tty{,S,USB,AMA,ACM}[0-9]*    rw,
  /dev/pts/[0-9]*    rw,
  /sys/dev/char     r,
  /sys/dev/char/**  r,
```


Restart apparmor

```
sudo systemctl restart apparmor
```

and try again
