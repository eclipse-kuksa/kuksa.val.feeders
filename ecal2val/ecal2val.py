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
Subscriber subscribing topics through ECAL communication and sending to KUKSA.val
'''

import sys
import time

import ecal.core.core as ecal_core
from ecal.core.subscriber import ProtoSubscriber

import proto_struct.vss_data_pb2 as vss_data_pb2

from kuksa_client.grpc import VSSClient
from kuksa_client.grpc import Datapoint


ecal_core.initialize(sys.argv, "Python Protobuf")

sub = ProtoSubscriber("vss_data_python_protobuf_topic", vss_data_pb2.VssData)


'''This callback function subscribes topics
   and writes the date in the protobuf
   to the data broker through the client.'''


def callback(topic_name, vss_data_proto_msg, time):
    with VSSClient('127.0.0.1', 55555) as client:
        if vss_data_proto_msg.data_int != 0:
            client.set_current_values({
                vss_data_proto_msg.description: Datapoint(vss_data_proto_msg.data_int),
            })
        elif vss_data_proto_msg.data_float != 0:
            client.set_current_values({
                vss_data_proto_msg.description: Datapoint(vss_data_proto_msg.data_float),
            })
        else:
            client.set_current_values({
                vss_data_proto_msg.description: Datapoint(0),
            })


sub.set_callback(callback)

while ecal_core.ok():
    time.sleep(1)

ecal_core.finalize()
