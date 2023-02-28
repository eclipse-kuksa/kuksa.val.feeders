# Copyright (c) 2022 Robert Bosch GmbH

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
