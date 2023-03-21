#!/bin/env python3

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

import logging
import sys
import time

import sensor_msgs
import std_msgs
from cyclonedds.domain import DomainParticipant
from cyclonedds.pub import DataWriter
from cyclonedds.topic import Topic
from pykml import parser

logger = logging.getLogger(__name__)


def read_kml_file(kml_filename):
    """Read kml and return array of coordinates."""
    logger.info("Reading KML file %s", kml_filename)
    with open(kml_filename, "r", encoding="utf8") as kml_file:
        return (
            parser.parse(kml_file)
            .getroot()
            .Document.Placemark.LineString.coordinates.text.split()
        )


class DDSPublisher:  # pylint: disable=too-few-public-methods
    """
    Sample DDS Publisher.

    Sends out "NavStatFix" message with values read from KML file every 1 second
    """

    def __init__(self, data):
        self.data = data
        participant = DomainParticipant()
        """
        OMG specification supports only '-' as a separator in topic names
        Currently cyclonedds does not support symbols '.' or '-' in the dds topic names
        Hence '_' is used. https://github.com/eclipse-cyclonedds/cyclonedds/issues/1393
        """
        topic = Topic(participant, "Nav_Sat_Fix", sensor_msgs.msg.NavSatFix)

        self.writer = DataWriter(participant, topic)

    def _publish(self, message):
        self.writer.write(message)

    def run(self):
        ctr = 0
        while 1:
            sec = int(time.time())
            nanosec = 2  # time.time_ns()
            print(f"sec : {sec}")
            print(f"nanosec : {nanosec}")
            header = std_msgs.msg.Header(
                stamp=std_msgs.msg.Time(sec=sec, nanosec=nanosec),
                frame_id=str(ctr),
            )
            latitude, longitutde, altitude = self.data[ctr].split(",")
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
                # pylint: disable=C0301
                position_covariance_type=sensor_msgs.msg.NavSatFix__COVARIANCE_TYPE_KNOWN,  # noqa: E501
            )
            self._publish(message)
            print("Published Message", message)
            time.sleep(1)
            ctr = (ctr + 1) % len(self.data)


def main():
    """Start the KML replay."""
    logger.info("Starting KML Replay...")
    kml_file = sys.argv[1:][0]
    data = read_kml_file(kml_file)
    pub_obj = DDSPublisher(data)
    pub_obj.run()


if __name__ == "__main__":
    main()
