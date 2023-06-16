#!/usr/bin/env python3

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

import contextlib
import logging
import time
from typing import Optional

# pylint: disable=unused-import
import sensor_msgs  # # noqa: F401

# F401 std_msgs module is used  to access the data classes
# pylint: disable=unused-import
import std_msgs  # # noqa: F401

from pathlib import Path

# F401 Vehicle module is used inside the eval block to access the data classes
# pylint: disable=unused-import
import Vehicle  # noqa: F401
from cyclonedds.core import Listener
from cyclonedds.domain import DomainParticipant
from cyclonedds.sub import DataReader
from cyclonedds.topic import Topic
from grpc import ChannelConnectivity
from kuksa_client.grpc import VSSClient, VSSClientError

from ddsproviderlib import databroker, vss2ddsmapper  # noqa: F401

log = logging.getLogger("ddsprovider")


class DdsListener(Listener):
    """Class used as callback for event based actions."""

    def __init__(
        self, provider: databroker.Provider, mapper: vss2ddsmapper.Vss2DdsMapper
    ):
        self.provider = provider
        self.mapper = mapper
        super().__init__()

    def on_data_available(self, reader):
        """Received data on the reader."""
        data = reader.take_next()
        topic_name = reader.topic.get_name()
        typename = self.mapper.dds2vss_dict[topic_name].get("typename")
        log.debug("Data received on topic: %s", topic_name)
        log.debug("Data : %s", data)
        log.debug("dataclass : %s", typename)

        # Only update in broker its connected
        for vss_signal_d in self.mapper.dds2vss_dict[topic_name].get("vsssignals"):
            # One dds topic can hold data for multiple vss data points
            element = vss_signal_d.get("element")
            vss_signal = vss_signal_d.get("vsssignal")
            log.debug("updating vss point %s", vss_signal)
            log.debug("Picking data from element: %s", element)
            # Update broker , TODO: Measure/evaluate making grpc calls in dds callback
            try:
                value = getattr(data, element)
                value = self.mapper.transform(topic_name, vss_signal, value)
                # Return value from update_datapoint is ignore as of now
                self.provider.update_datapoint(vss_signal, value)
            except AttributeError:
                log.error("Element does not exist in the data class")


def register_datapoints(
    provider: databroker.Provider, mapper: vss2ddsmapper.Vss2DdsMapper
):
    log.info("Registering datapoints...")

    # Set to registered, if failed will be reset in exception block
    for vss_signal in mapper.mapping:
        if not provider.register(
            vss_signal,
            mapper.mapping[vss_signal]["databroker"]["datatype"],
            mapper.mapping[vss_signal]["description"],
        ):
            return False

    return True


class Ddsprovider:
    """class to work with DDS feeder."""

    # pylint: disable=too-many-instance-attributes
    def __init__(self, root_ca_path: Optional[str] = None, tls_server_name: Optional[str] = None):
        self._shutdown = False
        self._provider = None
        self._registered = False
        self._connected = False
        self._subscribed = False
        self._reader = []
        self._listener = None
        self._mapper = None
        self._root_ca_path = root_ca_path
        self._tls_server_name = tls_server_name
        self._exit_stack = contextlib.ExitStack()

    async def start(self, databroker_address, grpc_metadata, mappingfile, token):
        """Actions done.

        1. Creates a GRPC channel to talk to databroker
        2. Subscribe to connectivity change in databroker
        3. Run the DDS feeder
        """
        log.info("Connecting to Data Broker using %s", databroker_address)
        self._mapper = vss2ddsmapper.Vss2DdsMapper(mappingfile)
        host, port = databroker_address.split(":")

        # If there is a path VSSClient will request a secure connection
        if self._root_ca_path:
            root_path = Path(self._root_ca_path)
        else:
            root_path = None

        try:
            vss_client = self._exit_stack.enter_context(
                VSSClient(host=host, port=port, token=token,
                          root_certificates=root_path, tls_server_name=self._tls_server_name)
            )
        except VSSClientError as kuksa_error:
            log.error(kuksa_error)
            return
        vss_client.channel.subscribe(
            self._on_broker_connectivity_change, try_to_connect=False
        )
        self._provider = databroker.Provider(vss_client, grpc_metadata)
        self._run()

    def stop(self):
        """Terminated ddsffeder."""
        log.info("Shutdown initiated")
        self._shutdown = True

    def _should_shutdown(self):
        return self._shutdown

    def _run(self):
        while not self._should_shutdown():
            if not self._connected:
                time.sleep(0.2)
            elif not self._subscribed:
                self._subscribe()
            else:
                pass

    def _subscribe(self):
        """Subscribe to all DDS topics specified in the mapping file."""
        log.info("Starting subscriber...")
        participant = DomainParticipant()
        assert self._mapper is not None
        assert self._provider is not None

        self._listener = DdsListener(self._provider, self._mapper)
        for topic_name, topicinfo_d in self._mapper.dds2vss_dict.items():
            dataclass_name = topicinfo_d.get("typename")
            log.debug(
                "Subscriberd to topic: %s using data class: %s",
                topic_name,
                dataclass_name,
            )
            topic = Topic(
                participant,
                topic_name,
                # data classes are imported under the Vehicle folder
                # Attention: find a better way than "eval", using eval is risky
                # pylint: disable=eval-used
                eval(dataclass_name),
            )
            # Create a reader for each topic
            self._reader.append(DataReader(participant, topic, listener=self._listener))
        self._subscribed = True

    def _on_broker_connectivity_change(self, connectivity):
        log.debug("Connectivity changed to: %s", connectivity)
        if connectivity in {
            ChannelConnectivity.READY,
            ChannelConnectivity.IDLE,
        }:
            # Can change between READY and IDLE. Only act if coming from
            # unconnected state
            if not self._connected:
                log.info("Connected to data broker")
                self._connected = True
                assert self._provider
                assert self._mapper
                self._registered = register_datapoints(self._provider, self._mapper)
                if not self._registered:
                    log.error("Failed to register datapoints")
        else:
            if self._connected:
                log.info("Disconnected from data broker")
            else:
                if connectivity == ChannelConnectivity.CONNECTING:
                    log.info("Trying to connect to data broker")
            self._connected = False
            self._registered = False
