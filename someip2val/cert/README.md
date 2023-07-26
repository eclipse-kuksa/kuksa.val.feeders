# Authorization in KUKSA.val databroker

More details on authorization in KUKSA.val can be found in [authorization.md](https://github.com/eclipse/kuksa.val/blob/master/doc/KUKSA.val_data_broker/authorization.md)

## Get required keys/tools from KUKSA.val

Some keys/tooling from KUKSA.val may be needed locally.
To get them, execute the following commands in someip2val/cert directory.

```bash
# get KUKSA.val key:
curl -sOL https://github.com/eclipse/kuksa.val/raw/master/kuksa_certificates/jwt/jwt.key
curl -sOL https://github.com/eclipse/kuksa.val/raw/master/kuksa_certificates/jwt/jwt.key.pub
# get tooling for signing tokens (optional):
curl -sOL https://github.com/eclipse/kuksa.val/raw/master/kuksa_certificates/jwt/createToken.py
curl -sOL https://github.com/eclipse/kuksa.val/raw/master/kuksa_certificates/jwt/requirements.txt
curl -sOL https://github.com/eclipse/kuksa.val/raw/master/kuksa_certificates/jwt/recreateJWTkeyPair.sh
```

For more details check KUKSA.val JWT tooling [README](https://github.com/eclipse/kuksa.val/blob/master/kuksa_certificates/README.md#java-web-tokens-jwt)

**NOTE:** Token examples there are not compatible with databroker, just check tools usage details.

## Authorization support in someip2val

someip2val authorizes to KUKSA.val using [someip2val.json](./someip2val.json).
It grants permission for registering datapoints, also provide and actuate datapoints on wiper relevant VSS paths.

There is already signed [someip2val.token](./someip2val.token) by default KUKSA.val key.

If you need to modify something in the payload, you can edit json file and then update the token with:

```bash
./createToken.py someip2val.json
```

### Authorization example setup

Described setup works with installed someip2val binaries, on a single host, using wiper-service example to simulate hardware wiper.

- Run databroker (in new terminal) with mapped `/cert` volume containing KUKSA.val public key:

```bash
docker run -it --rm --network host --name databroker \
    -v `$(git rev-parse --show-toplevel)/someip2val/cert`:/cert \
    ghcr.io/eclipse/kuksa.val/databroker:master --jwt-public-key /cert/jwt.key.pub
```

- Make sure someip2val is build and installed (`x86_64` arch is assumed):

```bash
cd $(git rev-parse --show-toplevel)/someip2val
./build-release.sh
```

- Run SOME/IP wiper-service example (in new terminal):

```bash
cd $(git rev-parse --show-toplevel)/someip2val/target/x86_64/release/install/bin
. setup-wiper-service.sh
./wiper-service --cycle 500
```

- Run someip2val with its token (in new terminal):

```bash
cd $(git rev-parse --show-toplevel)/someip2val/target/x86_64/release/install/bin
. setup-someip2val.sh
./someip_feeder --token $(git rev-parse --show-toplevel)/someip2val/cert/someip2val.token
```

**NOTE:** Wiper events can be verbose, to disable them `export WIPER_STATUS=0`, or use `export WIPER_STATUS=2` to make them on single line.

- Test actuator subscriber with kuksa-client:

```bash
# Use --pre if you rely on kuksa-client pre-releases
pip3 install -U kuksa-client

GRPC_ENABLE_FORK_SUPPORT=true kuksa-client --ip 127.0.0.1 --port 55555 --protocol grpc --insecure --token_or_tokenfile $(git rev-parse --show-toplevel)/someip2val/cert/someip2val.token
```

- Set 3 required values to trigger someip request:

```text
Test Client> setTargetValues Vehicle.Body.Windshield.Front.Wiping.System.Frequency=3 Vehicle.Body.Windshield.Front.Wiping.System.Mode=WIPE Vehicle.Body.Windshield.Front.Wiping.System.TargetPosition=100
```

- Check someip_feeder dumps for the following lines:

```text
# SomeipFeederAdapter::on_actuator_change: [info] updated target_values: 3
# SomeipFeederAdapter::on_actuator_change: [info] Sending WiperReq: { mode:WIPE, freq:3, targetPos:100 }
...
# SomeIPClient<someip_feeder>::SendRequest: [info] ### Sending Request to [6123.000b.0007] with payload: 03 00 00 c8 42 02
...
# SomeipFeederAdapter::on_someip_message: [info] Received Response from [6123.000b.0007], payload [00]

```
