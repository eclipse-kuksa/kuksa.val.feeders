########################################################################
# Copyright (c) 2022 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License 2.0 which is available at
# http://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
########################################################################

from setuptools import setup

setup(
    name="sensor_msgs",
    description="Message classes generated from .idl files via idlc",
    packages=["sensor_msgs", "sensor_msgs.msg", "std_msgs", "std_msgs.msg"],
    # Location of generated NavSatFix.py
    package_dir={
        "sensor_msgs": "sensor_msgs",
        "sensor_msgs.msg": "sensor_msgs/msg",
        "std_msgs": "std_msgs",
        "std_msgs.msg": "std_msgs/msg",
    },
)
