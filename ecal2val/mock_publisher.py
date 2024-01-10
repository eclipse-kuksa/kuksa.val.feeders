#! /usr/bin/env python3

########################################################################
# Copyright (c) 2023 Contributors to the Eclipse Foundation
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

'''
Publisher publishing vss topics via eCAL communication.
'''

import sys
import time

import ecal.core.core as ecal_core
from ecal.core.publisher import ProtoPublisher

import proto.proto_struct_pb2 as proto_struct_pb2

from cast_func import *


ecal_core.initialize(sys.argv, "ecal2val")

pub = ProtoPublisher("vss_topic", proto_struct_pb2.DataEntry)

'''
Reads arbitrary data from 'mock_data.txt'
and publishes it in the form of a protobuf message.
'''

while ecal_core.ok():
    with open("mock_data.txt", 'r', encoding='utf-8') as file:
        for line in file:
            path, value, data_type = line.split()

            entry = proto_struct_pb2.DataEntry()
            entry.path = path
            exec(f"entry.value.{data_type} = {data_type}(value)")
            entry.data_type = data_type

            pub.send(entry)
            print(f"{path} published")

            time.sleep(1)

ecal_core.finalize()
