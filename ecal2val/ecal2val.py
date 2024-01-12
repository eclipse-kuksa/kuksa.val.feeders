#! /usr/bin/env python3

########################################################################
# Copyright (c) 2024 Contributors to the Eclipse Foundation
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
Subscriber subscribing vss topics via ECAL communication
and sending it to KUKSA.val databroker.
'''

import sys
import time

import ecal.core.core as ecal_core
from ecal.core.subscriber import ProtoSubscriber

import proto.proto_struct_pb2 as proto_struct_pb2

from kuksa_client.grpc import Datapoint
from kuksa_client.grpc import DataEntry
from kuksa_client.grpc import EntryUpdate
from kuksa_client.grpc import Field
from kuksa_client.grpc import VSSClient


ecal_core.initialize(sys.argv, "ecal2val")

sub = ProtoSubscriber("vss_topic", proto_struct_pb2.DataEntry)

'''
This callback function subscribes topics
and writes the data to the databroker.
'''


def callback(topic_name, msg, time):
    with VSSClient('127.0.0.1', 55555) as client:
        entry = DataEntry(
            path=msg.path,
            value=Datapoint(value=eval(f"msg.value.{msg.data_type}")),
        )
        updates = (EntryUpdate(entry, (Field.VALUE,)),)

        client.set(updates=updates)

        print(f'{msg.path} subscribed & written')


sub.set_callback(callback)

while ecal_core.ok():
    time.sleep(1)

ecal_core.finalize()
