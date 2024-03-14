# PS4/PS5 - 2021 Formula 1  Feeder
### This demonstrator serves as a non-commercial research project, highlighting its focus on academic exploration rather than commercial endeavors.
The feeder consumes [F1 Telemetrydata](https://www.ea.com/able/resources/f1-2021/ps4/telemetry) as datasource and pushes data to the kuksa.val Databroker.
### Video Demo (Click to Watch!)
[![Demo Video](https://img.youtube.com/vi/7C_yHItbJNU/0.jpg)](https://www.youtube.com/watch?v=7C_yHItbJNU "Demo Video - Click to Watch!")

For the remaining parts of the demonstrator, except for the feeder contained in this repository, see
https://github.com/fraunhofer-iem/f1-telemetry-dashboard
### Feeder
The custom [VSS File](./VSS/vss.json) contains specification points for further Application use.\
The [`carTelemetry_feeder.ini`](./config/carTelemetry_feeder.ini)  contains `kuksa.val`, `listenerIPAddr` and `PS5_UDPPort` configuration.

Before starting the [F1 feeder](./carTelemetry_feeder.py), you need to start the `kuksa.val databroker` docker container by running the following command in the main project folder:
```
docker run -it -v ./VSS:/VSS --rm --net=host -p 127.0.0.1:8090:8090 -e LOG_LEVEL=ALL ghcr.io/eclipse/kuksa.val/databroker:master --insecure --vss /VSS/vss.json
```
This VSS folder, contains a custom vss.json file for this particular game.
## Install dependencies and execution

General Information: This Project was run on an Ubuntu VM and created in cooperation with [`Fraunhofer IEM`](https://www.iem.fraunhofer.de/) as a non-commercial research project.

#### carTelemetry_feeder.ini
```
a. The F1 telemetry port/IP number for communication has to be updated in the ./config/carTelemetry_feeder.ini file.

	> IP address of the Host/VM for example 192.168.178.154
	> Same with the Port: fore example 20778

b. The listenerIPAddr of the host/VM a also needs to be updated in the ./config/carTelemetry_feeder.ini file.

	> It has to match with the given IP in step a.

c. The PS5_UDPPort of the host/VM a also needs to be updated in the ./config/carTelemetry_feeder.ini file.

	> It has to match with the given Port in step a.

d. kuksa.val IP for the VSSClient has to be updated in the ./config/carTelemetry_feeder.ini file.

	> Normally set to 127.0.0.1.

e. kuksa.val port for the VSSClient has to be updated in the ./config/carTelemetry_feeder.ini file.

	> Normaly set to 55555.
```

#### Dependencies:
```
We need to install the python F1 Module

	> $pip install Telemetry-F1-2021

We also need to install kuksa-client (if not already done)

	> $pip install kuksa-client
```

#### PS5 Settings
```
PS5 Telemetry Settings:

	1. We have to enable the telemetry feature in the Game Settings.
	2. We can make note of the UDP port number (20778)
	3. We need to update the laptop/PC IP address for UDP communication.
```
#### Running the feeder

Now to run the feeder, execute the following command in your favorite Command Line Interface (Terminal):
```
python3 carTelemetry_feeder.py
```
#### What Data is sent?

Currently we are sending the following data:
```
Vehicle speed in kmh,
Vehicle engine rpm,
Vehicle fuel level in percent,
Wear level of each tire in percent,
Left and right wing damage in percent,
and last, Vehicle last Lap Time
```

## Authorization

[F1 feeder](./carTelemetry_feeder.py) will try to authenticate itself towards the KUKSA.val Server/Databroker if a token is given.
Note that the KUKSA.val Databroker by default does not require authentication.


### Troubleshouting
if the python feeder command fails:
```
(Errno 99 Cannot assign requested Address)

	1. use the Linux command 'ifconfig' in your terminal
	2. find the following line: enp0s3: inet 192.168.178.***
	--> and copy the IP into the ./config/carTelemetry_feeder.ini file
```
