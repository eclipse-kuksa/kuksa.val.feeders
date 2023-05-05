#!/bin/bash
set -eu
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
cd "$SCRIPT_DIR"
# moving files to tmp as the py files generated as on the same folder
# as the package name and would get deleted

# git clone Vehicle signal specification and vss-tools
cd /tmp
rm -rf /tmp/vehicle_signal_specification
rm -rf /tmp/vss-tools
git clone https://github.com/COVESA/vehicle_signal_specification.git -b v4.0
git clone https://github.com/COVESA/vss-tools.git -b v4.0

# copy spec folder from VSS to VSS tools
cp -R /tmp/vehicle_signal_specification/spec /tmp/vss-tools/spec
cd -
cd /tmp/vss-tools

# install dependencies for VSS-tools to covert spec to idl
pip install anytree
pip install graphql-core

# convert spec to idl
python3 vspec2ddsidl.py -I ./spec --no-uuid ./spec/VehicleSignalSpecification.vspec out.idl
cd -

# take all the idls into tmp folder
cp -r . /tmp/idls
cd /tmp/idls
cp /tmp/vss-tools/out.idl /tmp/idls/vss

idlc -l py -I std_msgs/msg -x final NavStatFix.idl
idlc -l py -I std_msgs/msg -x final NavSatStatus.idl
idlc -l py -I std_msgs/msg -x final std_msgs/msg/Header.idl
idlc -l py -I std_msgs/msg -x final std_msgs/msg/Time.idl

cd vss
idlc -l py -x final out.idl

pip install .

rm -rf build
rm -rf ./*.egg-info
rm -rf Vehicle
cd -


pip install .
rm -rf build
rm -rf sensor_msgs
rm -rf std_msgs
rm -rf ./*.egg-info
rm -rf build
