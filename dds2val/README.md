# DDS Provider

The DDS provider provides data from an DDS middleware/API. For further understanding of the DDS middleware/API see [this](https://www.dds-foundation.org/what-is-dds-3/). The DDS provider only works with the KUKSA Databroker. The KUKSA C++ Server is not supported.

## How to build

### local build

1. `python3 -m venv env && source env/bin/activate`
2. `pip install -r requirements/requirements.txt`
3. `chmod u+x ddsproviderlib/idls/generate_py_dataclass.sh`
4. `./ddsproviderlib/idls/generate_py_dataclass.sh`

### build image (suggested)

1. `docker build -f Dockerfile --progress=plain --build-arg TARGETPLATFORM=linux/amd64 -t ddsprovider:latest .`

### KML replay

*This requires that you already have created the virtual environment as described in "local build" above*

1. `source env/bin/activate`
2. `pip install -r requirements/requirements-kml.txt`
3. `cd kml && python3 dds_kmlreplay.py directions.kml`

## How to run

Choose from local build or containerization via docker.
These steps are necessary:

1. Run an instance of databroker aka: `docker run -it --rm --net=host ghcr.io/eclipse/kuksa.val/databroker:master`
2. Start the KML replay with an active local python virtual environment (see [KML replay section](#kml-replay))
3. Start the DDS provider with either: `docker run --rm -it --net=host ddsprovider:latest` or with an active local python virtual environment: `python3 ddsprovider.py`

## Configure the DDS provider

Configuration for the DDS provider is solved through setting environment variables. Have a look at the table below.

| Environment variable          | default value | description                                                                                                                                      |
| ----------------------------- | ------------- | ------------------------------------------------------------------------------------------------------------------------------------------------ |
| VDB_ADDRESS                   | 127.0.0.1     | Address where to look for KUKSA.val Databroker                                                                                                   |
| VDB_PORT                      | 55555         | On which port the KUKSA.val Databroker is expected.                                                                                              |
| MAPPING_FILE                  | mapping/latest/mapping.yml   | Place of mapping file from DDS to VSS                                                                                             |
| TOKEN                         | None          | JWT token which will get used to authorize to databroker; You can set on linux through `export TOKEN=$(< PATH_TO_kuksa.val/jwt/provide-all.token)` or `export TOKEN=<content of a file>`|
| VDB_ROOT_CA_PATH              | None          | Path to root CA. If defined the client will attempt to use a secure connection and identify the server using this certificate. |
| VDB_TLS_SERVER_NAME           | None          | TLS server name, may be needed if addressing a server by IP-name |
| VEHICLEDATABROKER_DAPR_APP_ID | None          | DAPR ID for Vehicle App to look for. For more information to Vehicle Apps visit [Velocitas](https://eclipse-velocitas.github.io/velocitas-docs/) |
| DAPR_GRPC_PORT                | None          | If [DAPR](https://dapr.io/) gets used port of DAPR Sidecar. Overwrites VDB_PORT variable. Deprecated - please use  VDB_PORT instead              |

## Overall sequence

```mermaid
sequenceDiagram
title DDS on SDV
box LightBlue Container-1
participant databroker
end

dddsprovider -->> databroker : grpc_connect
alt on connection
    ddsprovider -->> databroker : register_datapoints
    ddsprovider -->> DDS_network : subscribe to topics
    ddspublisher1 -->> DDS_network : publish dds message
    ddspublisher2 -->> DDS_network : publish dds message
end
box LightBlue Container-2
participant ddspublisher1
end
box LightBlue Container-3
participant ddspublisher2
end
alt on data reception
    ddsprovider -->> databroker : update_datapoint
end
```

## How to run the tests

1. create virtual python environment (`python3 -m venv testEnv && source testEnv/bin/activate && pip install -r requirements/requirements.txt -r requirements/requirements-test.txt -r requirements/requirements-kml.txt`)
2. terminal 2: `source testEnv/bin/activate && pytest --html=report.html --self-contained-html --cov=. tests/* --cov-report html --cov-report xml`

## TLS

If connecting to a KUKSA.val Databroker that require a secure connection you must specify which root certificate to
use to identify the Server by the `VDB_ROOT_CA_PATH` variable. If your (test) setup use the KUKSA.val example
certificates you must give [CA.pem](https://github.com/eclipse/kuksa.val/blob/master/kuksa_certificates/CA.pem)
as root CA. The server name must match the name in the certificate provided by KUKSA.val Databroker.
Due to a limitation in the gRPC client, if connecting by IP-address you may need to give a name listed in the certificate
by the `VDB_TLS_SERVER_NAME` variable. The example server certificate lists the names `Server` and `localhost`,
so one of those names needs to be specified if connecting to `127.0.0.1`. An example is shown below:

```
VDB_ROOT_CA_PATH=/home/user/kuksa.val/kuksa_certificates/CA.pem VDB_TLS_SERVER_NAME=Server python3 ddsprovider.py
```
