# Usage of kuksa.val replay feature

*Note: Both [KUKSA Server](https://github.com/eclipse/kuksa.val/tree/master/kuksa-val-server) and*
*Replay are deprecated and will reach End-of-Life 2024-12-31!*
*For [KUKSA Databroker](https://github.com/eclipse/kuksa.val/tree/master/kuksa_databroker) the*
*[KUKSA CSV Provider](https://github.com/eclipse-kuksa/kuksa-csv-provider) offer similar functionality.*

![kuksa.val Logo](../doc/img/logo.png)

Once you recorded your server in- and outputs to your record file using the [record feature](https://github.com/eclipse/kuksa.val/blob/master/kuksa-val-server/src/VssDatabase_Record.cpp) the replay script can replay the same data with exact timing into the `kuksa.val` server.
An example log can be found in [testlog.csv](testlog.csv)

## Usage

### Preparations

The Replay tool relies on that `kuksa-client` is installed.

```
$ pip install kuksa-client
```

You must also clone [kuksa.val](https://github.com/eclipse/kuksa.val) if the KUKSA.val Server you want to connect to
use KUKSA.val example certificates and tokens.


### Record a new file

Start `kuksa.val` server providing a record level, for example:

``` bash
$ ./kuksa-val-server --record=recordSetAndGet
```
Provide a different path than the default one using

```
$ ./kuksa-val-server --record=recordGetAndSet --record-path=/path/to/logs
```

### Configuration

Please configure [config.ini](config.ini) to set log file path and select your Replay mode.
The example configuration file assumes that your KUKSA.val Server use KUKSA.val example certificates and tokens and
that you have cloned [kuksa.val](https://github.com/eclipse/kuksa.val)
in parallel to `kuksa.val.feeders`, i.e. that you can find `kuksa.val` at `../../kuksa.val`.
If this is not the case you need to change the configuration to reflect your actual setup.

#### Available replay modes are:

|mode|action|
|-|-|
| Set | Replay Set Value only|
| SetGet | Replay Get Value and Set Value to the server |


### Run Replay

Replay can be run like this.

```
~/kuksa.val.feeders/replay$ python _replay.py
connect to wss://127.0.0.1:8090
Websocket connected securely.
Connected successfully
Replaying data from testlog.csv
Replay successful
```

## Limitations

* Replay only support communication with KUKSA.val Server using Websocket, it does not support communication with KUKSA.val Databroker
