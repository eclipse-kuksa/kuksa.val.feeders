# Kuksa Feeders
![kuksa.val Logo](./doc/img/logo.png)

This are data feeders for VSS based systems. The feeders here work with [KUKSA.val](https://github.com/eclipse/kuksa.val)

Name | Description
---- | -----------
[GPS Provider](https://github.com/eclipse-kuksa/kuksa-gps-provider)        | GPS data source for KUKSA.val Server and KUKSA.val Databroker *- NOTE: Moved to new repo!*
[CAN Provider (DBC feeder)](https://github.com/eclipse-kuksa/kuksa-can-provider)        | DBC feeder for for KUKSA.val Server and KUKSA.val Databroker *- NOTE: Moved to new repo!*
[SOME/IP feeder](./someip2val) | SOME/IP feeder for KUKSA.val Databroker
[DDS Provider](https://github.com/eclipse-kuksa/kuksa-dds-provider)      | DDS provider for KUKSA.val Databroker *- NOTE: Moved to new repo!*
[Replay](./replay)             | KUKSA.val Server replay script for previously recorded files, created by providing KUKSA.val Server with `--record` argument
[CSV provider](./csv_provider) | Script to replay VSS signals to KUKSA.val Databroker as defined in a CSV-file

## Pre-commit set up
This repository is set up to use [pre-commit](https://pre-commit.com/) hooks.
Use `pip install pre-commit` to install pre-commit.
After you clone the project, run `pre-commit install` to install pre-commit into your git hooks.
Pre-commit will now run on every commit.
Every time you clone a project using pre-commit running pre-commit install should always be the first thing you do.
