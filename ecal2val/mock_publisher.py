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
Publisher publishing topics via protobuf message through eCAL communication.
'''

import sys
import time

import ecal.core.core as ecal_core
from ecal.core.publisher import ProtoPublisher

import proto_struct.vss_data_pb2 as vss_data_pb2


ecal_core.initialize(sys.argv, "Python Protobuf")

pub = ProtoPublisher("vss_data_python_protobuf_topic", vss_data_pb2.VssData)

'''Reads arbitrary data from 'mock_data.txt'
   and publishes it in the form of a protobuf message.'''
while ecal_core.ok():
    with open("mock_data.txt", 'r', encoding='utf-8') as file:
        for line in file:
            description, data = line.rstrip().split()

            protobuf_message = vss_data_pb2.VssData()
            protobuf_message.description = description
            protobuf_message.data_int = 0
            protobuf_message.data_float = 0

            try:
                protobuf_message.data_int = int(data)
            except ValueError:
                protobuf_message.data_float = float(data)

            pub.send(protobuf_message)
            print("{} published".format(description))

            time.sleep(1)

ecal_core.finalize()
