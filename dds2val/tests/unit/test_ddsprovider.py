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

import unittest
from pathlib import Path
from unittest import mock

import grpc
from cyclonedds.sub import DataReader

from ddsproviderlib.databroker import Provider
from ddsproviderlib.helper import Ddsprovider, DdsListener
from ddsproviderlib.vss2ddsmapper import Vss2DdsMapper


class TestDDSProvider(unittest.TestCase):
    """Test case to registration of VSS data points."""

    def setUp(self):
        self.mappingfile = (
            Path(__file__).parent.parent.parent
            / "mapping/latest/mapping.yml"
        )
        self.grpcmetadata = "vehicledatabroker"
        self.dapr_grpc_port = "127.0.0.1:55555"

    def create_dds_feeder(self):
        ddsprovider = Ddsprovider()
        ddsprovider._mapper = Vss2DdsMapper(self.mappingfile)
        ddsprovider._provider = Provider(grpc.insecure_channel("127.0.0.1:55555"), None)
        return ddsprovider

    @mock.patch("ddsproviderlib.databroker.Provider.register")
    def test_broker_connectivity_idle_first_time(self, mocked_register):
        """Checks if registered values match the mapping file."""
        calls = [
            mock.call(
                "Vehicle.Cabin.Light.PerceivedAmbientLight", "INT8", "Ambient light value"
            ),
            mock.call("Vehicle.CurrentLocation.Latitude", "FLOAT", "Latitude value"),
            mock.call("Vehicle.CurrentLocation.Longitude", "FLOAT", "Longitude value"),
            mock.call("Vehicle.CurrentLocation.Altitude", "FLOAT", "Altitude value"),
        ]

        ddsprovider = self.create_dds_feeder()
        ddsprovider._on_broker_connectivity_change(grpc.ChannelConnectivity.IDLE)

        self.assertTrue(ddsprovider._registered)
        mocked_register.assert_has_calls(calls, any_order=True)

    @mock.patch("ddsproviderlib.databroker.Provider.register")
    def test_broker_connectivity_idle_register_failure(self, mocked_register):
        """All tests related to broker connectivity."""

        ddsprovider = self.create_dds_feeder()
        mocked_register.return_value = False
        ddsprovider._on_broker_connectivity_change(grpc.ChannelConnectivity.IDLE)
        assert ddsprovider._registered is False

    def test_broker_connectivity_connecting(self):
        # grpc channel connecting
        ddsprovider = self.create_dds_feeder()
        ddsprovider._on_broker_connectivity_change(grpc.ChannelConnectivity.CONNECTING)
        self.assertFalse(ddsprovider._registered)
        self.assertFalse(ddsprovider._connected)

    def test_broker_connectivity_transient_failure(self):
        # grpc channel disconnected
        ddsprovider = self.create_dds_feeder()
        ddsprovider._connected = True
        ddsprovider._on_broker_connectivity_change(
            grpc.ChannelConnectivity.TRANSIENT_FAILURE
        )
        self.assertFalse(ddsprovider._registered)
        self.assertFalse(ddsprovider._connected)

    @mock.patch("ddsproviderlib.helper.DataReader")
    @mock.patch("ddsproviderlib.helper.Topic")
    def test_broker_subscribe(self, mocked_topic, _mocked_DataReader):
        """All tests related to dds subscribe."""

        ddsprovider = self.create_dds_feeder()

        ddsprovider._subscribe()

        self.assertTrue(len(ddsprovider._reader) > 0)

    @mock.patch("ddsproviderlib.helper.Ddsprovider._subscribe")
    def test_run_shutdown_triggered(self, mock_subscribe):
        """When shutdown flag is set subscribe should not be called, directly terminate."""
        ddsprovider = self.create_dds_feeder()
        ddsprovider.stop()

        ddsprovider._run()

        self.assertEqual(mock_subscribe.call_count, 0)

    @mock.patch("ddsproviderlib.helper.Ddsprovider._subscribe")
    def test_run_subscribes_when_connected_and_not_subscribed(self, mock_subscribe):
        ddsprovider = self.create_dds_feeder()
        ddsprovider._connected = True

        # Make sure main loop is executed only once
        mock_subscribe.side_effect = ddsprovider.stop

        ddsprovider._run()

        # Check if sleep was called
        self.assertEqual(mock_subscribe.call_count, 1)

    @mock.patch("ddsproviderlib.helper.time.sleep")
    def test_run_sleeps_when_not_connected(self, mock_sleep):
        ddsprovider = self.create_dds_feeder()

        def stop_ddsprovider(*unused_sleep_arguments):
            ddsprovider.stop()

        # Make sure main loop is executed only once
        mock_sleep.side_effect = stop_ddsprovider

        ddsprovider._run()

        # Check if sleep was called
        self.assertEqual(mock_sleep.call_count, 1)

    @mock.patch("ddsproviderlib.helper.Ddsprovider._subscribe")
    @mock.patch("ddsproviderlib.helper.time.sleep")
    @mock.patch("ddsproviderlib.helper.Ddsprovider._should_shutdown")
    def test_run_does_nothing_when_connected_and_subscribed(
        self, mock_should_shutdown, mock_sleep, mock_subscribe
    ):
        ddsprovider = self.create_dds_feeder()

        def stop_after_3_iterations():
            return mock_should_shutdown.call_count > 3

        mock_should_shutdown.side_effect = stop_after_3_iterations

        ddsprovider._connected = True
        ddsprovider._subscribed = True

        ddsprovider._run()

        # Check if sleep and subscribe are never called
        self.assertEqual(mock_sleep.call_count, 0)
        self.assertEqual(mock_subscribe.call_count, 0)

    def test_stop(self):
        ddsprovider = self.create_dds_feeder()
        ddsprovider.stop()
        self.assertTrue(ddsprovider._should_shutdown())

    @mock.patch("ddsproviderlib.databroker.Provider")
    @mock.patch("ddsproviderlib.helper.getattr")
    def test_on_data_available(self, mock_getattr, mock_provider):
        # Check behaviour when dds type class is not valid

        listener = DdsListener(
            provider=mock_provider,
            mapper=Vss2DdsMapper(self.mappingfile),
        )

        reader = mock.Mock(spec=DataReader)
        reader.take_next.return_value = 10
        reader.topic.get_name.return_value = "Cabin_Light_Ambient_Light"

        mock_getattr.side_effect = AttributeError()

        listener.on_data_available(reader=reader)
        mock_provider.assert_not_called()


if __name__ == "__main__":
    unittest.main()
