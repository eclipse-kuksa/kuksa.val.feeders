# DDS Feeder

## How to build
### local build

1. ```python3 -m venv env && source env/bin/activate```
2. ```pip install -r requirements.txt```
3. ```./idls/generate_py_dataclass.sh```

### build image (suggested)

1. ```docker build -f Dockerfile --progress=plain --build-arg TARGETPLATFORM=linux/amd64 -t ddsfeeder:latest .```

### KML replay

1. ```python3 -m venv env && source env/bin/activate```
2. ```pip install -r requirements-kml.txt```

## How to run
Choose from local build or contanerization via docker.
These steps are necessary:
1. Run an instance of databroker aka: ```docker run -it --rm --net=host ghcr.io/eclipse/kuksa.val/databroker:master```
2. Start the KML replay with an active local python virtual environment: ```python3 dds_kmlreplay.py directions.kml```
3. Start the DDS feeder with either: ```docker run --rm -it --net=host ddsfeeder:latest``` or with an active local python virtual environment: ```python3 ddsfeeder.py```