# PS4/PS5 - Formula 1, 2021 Feeder
The feeder consumes [F1 Telemetrydata](https://www.ea.com/able/resources/f1-2021/ps4/telemetry) as datasource and pushes data to the kuksa.val Databroker.

The custom [VSS File](./VSS/vss.json) contains specification points for further Application use.
The [`carTelemetry_feeder.ini`](./config/carTelemetry_feeder.ini)  contains `kuksa.val`, `listenerIPAddr`, `PS5_UDPPort` and `carTelemetry` configuration.

Before starting the [F1 feeder](./carTelemetry_feeder.py), you need to start `kuksa.val databroker` docker by running the following command :
```
docker run -it -v ./VSS:/VSS --rm --net=host -p 127.0.0.1:8090:8090 -e LOG_LEVEL=ALL ghcr.io/eclipse/kuksa.val/databroker:master --insecure --vss /VSS/vss.json
```
## Install dependencies and execution

General Information: This Project was run on an Ubuntu VM in cooperation with [`Fraunhofer IEM`](https://www.iem.fraunhofer.de/) .

#### carTelemetry_feeder.ini
```
a. The F1 telemetry port number for communication is updated in the ./config/carTelemetry_feeder.ini file.
	F1 - Telemetry - Port: 20778
	
b. The listener host address also needs to be updated in the ./config/carTelemetry_feeder.ini  file
```

#### Dependencies:
```
We need to install the python F1 Module
$pip install Telemetry-F1-2021
```

#### PS5 Settings
```
PS5 Telemetry Settings:

a. We have to enable the telemetry feature in the Game Settings.
b. We can make note of the UDP port number (20778)
c. We need to update the laptop/PC IP address for UDP communication.
```
#### Running the feeder

Now to run the feeder execute the following command in your favorite Command Line Interface (Terminal):
```
python3 carTelemetry_feeder_copy.py
```
#### What Data is sent?

Currently we are sending the following data:
```
Vehicle Speed in kmh,
Vehicle Engine RPM,
Vehicle Fuel Level in percent,
Wear level of each Tire in percent,
Left and right Wing dmg in percent,
and last Vehicle last Lap Time
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
--> and copy the IP into the [`carTelemetry_feeder.ini`](./config/carTelemetry_feeder.ini) file
```
