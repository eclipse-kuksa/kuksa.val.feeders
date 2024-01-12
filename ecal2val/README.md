# eCAL Feeder
The purpose of this implementation is to input data received via `eCAL` into a `KUKSA.val` databroker. The topics transmitted by eCAL are in the form of protobuf, and based on the VSS description and data outlined in this format, it is possible to provide data to the databroker via kuksa_client.

## Usage
1. Install Python requirements for both eCAL and KUKSA.val

```
sudo add-apt-repository ppa:ecal/ecal-5.12
sudo apt-get update
sudo apt-get install ecal
sudo apt install python3-ecal5

pip install kuksa-client

```

2. Generate proto_struct_pb2.py in proto directory with following method

```
sudo apt-get install protobuf-compiler

protoc --python_out=. proto_struct.proto

```

3. Use the following command to run the ecal2val.py

```
export PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION=python

python3 ecal2val.py

```

This assumes a running `KUKSA.val` databroker at `127.0.0.1:55555`.

4. For testing, run the mock_publisher.py

```
export PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION=python

python3 mock_publisher.py

```

Modify proto file to utilize more specific information.

This was successfully tested on Ubuntu 20.04 and eCAL 5.12.
