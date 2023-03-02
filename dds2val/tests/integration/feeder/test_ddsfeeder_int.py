#################################################################################
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
#################################################################################

import os
import signal
import threading
import time
import subprocess

import pytest
import sensor_msgs  # # noqa: F401
import std_msgs
from cyclonedds.domain import DomainParticipant
from cyclonedds.pub import DataWriter
from cyclonedds.topic import Topic
from kuksa_client.grpc.aio import VSSClient, VSSClientError

from ....src import ddsfeeder as ddssrc


async def run_dds_feeder_and_send_dds_msg(data, topic_name, topic_type):
    # Run this after some delay, while dds feeder is running
    def parallel_thread():
        send_dds_messages(data, topic_name, topic_type)
        time.sleep(1)
        os.kill(os.getpid(), signal.SIGINT)

    threading.Timer(5, parallel_thread).start()

    # Run dds feeder until SIGINT signal via timer.
    os.environ["VEHICLEDATABROKER_DAPR_APP_ID"] = "vehicledatabroker"
    await ddssrc.main()


@pytest.mark.asyncio
async def test_ddsfeeder_start():
    "Test case to verify data send on dds bus is received in broker"

    databroker = subprocess.Popen(['docker', 'run', '--net=host', 'ghcr.io/eclipse/kuksa.val/databroker:master'], stdin=subprocess.PIPE,
                     stdout=subprocess.PIPE)
    # check if darabroker is up and running then progress
    building = True
    while(building):
        try:    
                async with VSSClient('127.0.0.1', 55555) as client:
                    building = False
        except VSSClientError:
            time.sleep(2)

    await run_dds_feeder_and_send_dds_msg(
        create_NavSatFix_dds_messages(["52.25,12.25,0"]),
        "Nav_Sat_Fix",
        sensor_msgs.msg.NavSatFix,
    )

    # Verify data received in broker is the same as send above
    async with VSSClient('127.0.0.1', 55555) as client:
        response = await client.get_current_values(["Vehicle.CurrentLocation.Latitude"],)
    lat = response["Vehicle.CurrentLocation.Latitude"].value
    assert lat == 52.25
    databroker.terminate()


def create_NavSatFix_dds_messages(data):
    for ctr, datapoint in enumerate(data):
        sec = int(time.time())
        nanosec = 2
        header = std_msgs.msg.Header(
            stamp=std_msgs.msg.Time(sec=sec, nanosec=nanosec),
            frame_id=str(ctr),
        )
        latitude, longitutde, altitude = datapoint.split(",")
        status = sensor_msgs.msg.NavSatStatus(
            status=sensor_msgs.msg.NavSatStatus__STATUS_FIX,
            service=sensor_msgs.msg.NavSatStatus__SERVICE_GPS,
        )
        message = sensor_msgs.msg.NavSatFix(
            header=header,
            status=status,
            latitude=float(latitude),
            longitude=float(longitutde),
            altitude=float(altitude),
            position_covariance=[1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9],
            position_covariance_type=sensor_msgs.msg.NavSatFix__COVARIANCE_TYPE_KNOWN,
        )
        yield message


def send_dds_messages(data, topic_name, topic_type):
    participant = DomainParticipant()
    topic = Topic(participant, topic_name, topic_type)
    writer = DataWriter(participant, topic)

    for message in data:
        writer.write(message)
