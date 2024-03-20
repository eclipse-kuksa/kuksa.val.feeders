# Kuksa Feeders and Providers
![kuksa.val Logo](./doc/img/logo.png)

*Note: This is a legacy repository. Most providers has been moved to new repositories!*


Name | Description
---- | -----------
[Replay](./replay)             |[KUKSA Server](https://github.com/eclipse/kuksa.val/tree/master/kuksa-val-server) replay script for previously recorded files, created by running KUKSA Server with `--record` argument **Note: Replay is deprecated and will reach End-of-Life 2024-12-31!**

## Providers moved to new repositories


* [GPS Provider](https://github.com/eclipse-kuksa/kuksa-gps-provider)
* [CAN Provider (DBC feeder)](https://github.com/eclipse-kuksa/kuksa-can-provider)
* [SOME/IP Provider](https://github.com/eclipse-kuksa/kuksa-someip-provider)
* [DDS Provider](https://github.com/eclipse-kuksa/kuksa-dds-provider)
* [CSV Provider](https://github.com/eclipse-kuksa/kuksa-csv-provider)
* [eCAL Provider](https://github.com/eclipse-kuksa/kuksa-incubation/tree/main/ecal2val)
* [PS4/PS5 - 2021 Formula Provider](https://github.com/eclipse-kuksa/kuksa-incubation/tree/main/fone2val)

## Pre-commit set up
This repository is set up to use [pre-commit](https://pre-commit.com/) hooks.
Use `pip install pre-commit` to install pre-commit.
After you clone the project, run `pre-commit install` to install pre-commit into your git hooks.
Pre-commit will now run on every commit.
Every time you clone a project using pre-commit running pre-commit install should always be the first thing you do.
