# DDS Provider

The DDS provider provides data from an DDS middleware/API. For further understanding of the DDS middleware/API see [this](https://www.dds-foundation.org/what-is-dds-3/).

## How to build

### local build

1. `python3 -m venv env && source env/bin/activate`
2. `pip install -r requirements.txt`
3. `./idls/generate_py_dataclass.sh`

### build image (suggested)

1. `docker build -f Dockerfile --progress=plain --build-arg TARGETPLATFORM=linux/amd64 -t ddsprovider:latest .`

### KML replay

1. `python3 -m venv env && source env/bin/activate`
2. `pip install -r requirements-kml.txt`

## How to run

Choose from local build or contanerization via docker.
These steps are necessary:

1. Run an instance of databroker aka: `docker run -it --rm --net=host ghcr.io/eclipse/kuksa.val/databroker:master`
2. Start the KML replay with an active local python virtual environment: `cd kml && python3 dds_kmlreplay.py directions.kml`
3. Start the DDS provider with either: `docker run --rm -it --net=host ddsprovider:latest` or with an active local python virtual environment: `python3 ddsprovider.py`

## Overall sequence

```mermaid
title DDS on SDV
box "Container-1"
participant databroker
end box
databroker <-- ddsprovider : grpc_connect
alt on connection
ddsprovider --> databroker : register_datapoints
ddsprovider --> DDS_network : subscribe to topics
box "Container-2"
participant ddspublisher1
end box
box "Container-3"
participant ddspublisher2
end box
ddspublisher1 --> DDS_network : publish dds message
ddspublisher2 --> DDS_network : publish dds message
alt on data reception
ddsprovider --> databroker : update_datapoint
end note
end
```

## How to run the tests

1. create virtual python environment (`python3 -m venv testEnv && source testEnv/bin/activate && pip install -r requirements/requirements.txt requirements/requirements.txt requirements/requirements-kml.txt`)
2. terminal 2: `source testEnv/bin/activate && pytest --html=report.html --self-contained-html --cov=. tests/* --cov-report html --cov-report xml`
