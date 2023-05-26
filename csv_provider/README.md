# VSS Provider
The aim of this script is to provide and replay VSS data into a `kuksa.val` databroker.
Therefore, it takes a CSV-file, containting pre-defined sequences of VSS signals including timing delays, and provides it to the 'kuksa.val' databroker.

## Usage
The provider requires an installation of Python in version 3 and can be executed with the following commands:

```
pip install -r requirements.txt
python provider.py
```

This assumes a running `kuksa.val` databroker at `127.0.0.1:55555` and a file named `signals.csv` containing the signals to apply. See the section [Arguments](#arguments) for more details on possible arguments and default values.

The provider uses the [kuksa_client]() Python implementation which you need to install on your system, e.g., by applying the [requirement.txt](requirements.txt) with pip.

## Arguments
You can start the provider with the following arguments on a command line:

| short argument | long argument | environment variable | description | default value |
|---- | ---- | ---- |----- | ----|
|-f| --file | PROVIDER_SIGNALS_FILE | This indicates the CSV-file containing the signals to update in the `kuksa.val` databroker. | signals.csv |
| -a | --address | KUKSA_DATA_BROKER_ADDR | This indicates the address of `kuksa.val` databroker to connect to. | 127.0.0.1 |
| -p | --port | KUKSA_DATA_BROKER_PORT | This indicates the port of the `kuksa.val` databroker to connect to. | 55555 |
| -i | --infinite | PROVIDER_INFINITE | If the flag is set, the provider loops over the file until stopped, otherwise the file gets processed once. | not present/False
| -l | --log | PROVIDER_LOG_LEVEL | This sets the logging level. Possible values are: DEBUG, INFO, DEBUG, WARNING, ERROR, CRITICAL | WARNING

## CSV File
An example CSV-files is available in [signals.csv](signals.csv) where an example line is:

```
current,Vehicle.Speed,48,1
```

The delimiter for the CSV-file is the ',' sign. The first line is interpreted as header and not as data.

Each line in the csv file consists of the following elements:

| header | description | example |
| -- | -----------| --|
| field | indicates whether to update the current value (`current`) or the target value (`target`) for the signal. | current |
| signal | the name of the signal to update | Vehicle.Speed
| value | the new value | 48 |
| delay | Indicates the time in seconds which the provider shall wait after processing this signal. This way one can emulate the change of signals over time. | 0 |
