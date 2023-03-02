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

from ...src.databroker import Provider
from ...src.ddsfeeder import Ddsfeeder, DdsListener
from ...src.vss2ddsmapper import Vss2DdsMapper


class TestDDSFeeder(unittest.TestCase):
    """Test case to registration of VSS data points."""

    def setUp(self):
        self.mappingfile = (
            Path(__file__).parent.parent.parent
            / "src"
            / "mapping.yml"
        )
        self.grpcmetadata = "vehicledatabroker"
        self.dapr_grpc_port = "127.0.0.1:55555"

    def create_dds_feeder(self):
        ddsfeeder = Ddsfeeder()
        ddsfeeder._mapper = Vss2DdsMapper(self.mappingfile)
        ddsfeeder._provider = Provider(grpc.insecure_channel("127.0.0.1:55555"), None)
        return ddsfeeder

    @mock.patch("dds2val.src.databroker.Provider.register")
    def test_broker_connectivity_idle_first_time(self, mocked_register):
        """Checks if registered values match the mapping file."""
        calls = [
            mock.call(
                "Vehicle.Cabin.Lights.AmbientLight", "INT8", "Ambient light value"
            ),
            mock.call("Vehicle.CurrentLocation.Latitude", "FLOAT", "Latitude value"),
            mock.call("Vehicle.CurrentLocation.Longitude", "FLOAT", "Longitude value"),
            mock.call("Vehicle.CurrentLocation.Altitude", "FLOAT", "Altitude value"),
        ]

        ddsfeeder = self.create_dds_feeder()
        ddsfeeder._on_broker_connectivity_change(grpc.ChannelConnectivity.IDLE)

        self.assertTrue(ddsfeeder._registered)
        mocked_register.assert_has_calls(calls, any_order=True)

    @mock.patch("dds2val.src.databroker.Provider.register")
    def test_broker_connectivity_idle_register_failure(self, mocked_register):
        """All tests related to broker connectivity."""

        ddsfeeder = self.create_dds_feeder()
        mocked_register.return_value = False
        ddsfeeder._on_broker_connectivity_change(grpc.ChannelConnectivity.IDLE)
        assert ddsfeeder._registered is False

    def test_broker_connectivity_connecting(self):
        # grpc channel connecting
        ddsfeeder = self.create_dds_feeder()
        ddsfeeder._on_broker_connectivity_change(grpc.ChannelConnectivity.CONNECTING)
        self.assertFalse(ddsfeeder._registered)
        self.assertFalse(ddsfeeder._connected)

    def test_broker_connectivity_transient_failure(self):
        # grpc channel disconnected
        ddsfeeder = self.create_dds_feeder()
        ddsfeeder._connected = True
        ddsfeeder._on_broker_connectivity_change(
            grpc.ChannelConnectivity.TRANSIENT_FAILURE
        )
        self.assertFalse(ddsfeeder._registered)
        self.assertFalse(ddsfeeder._connected)

    @mock.patch("dds2val.src.ddsfeeder.DataReader")
    @mock.patch("dds2val.src.ddsfeeder.Topic")
    def test_broker_subscribe(self, mocked_topic, _mocked_DataReader):
        """All tests related to dds subscribe."""

        ddsfeeder = self.create_dds_feeder()

        ddsfeeder._subscribe()

        self.assertTrue(len(ddsfeeder._reader) > 0)

    @mock.patch("dds2val.src.ddsfeeder.Ddsfeeder._subscribe")
    def test_run_shutdown_triggered(self, mock_subscribe):
        """When shutdown flag is set subscribe should not be called, directly terminate."""
        ddsfeeder = self.create_dds_feeder()
        ddsfeeder.stop()

        ddsfeeder._run()

        self.assertEqual(mock_subscribe.call_count, 0)

    @mock.patch("dds2val.src.ddsfeeder.Ddsfeeder._subscribe")
    def test_run_subscribes_when_connected_and_not_subscribed(self, mock_subscribe):
        ddsfeeder = self.create_dds_feeder()
        ddsfeeder._connected = True

        # Make sure main loop is executed only once
        mock_subscribe.side_effect = ddsfeeder.stop

        ddsfeeder._run()

        # Check if sleep was called
        self.assertEqual(mock_subscribe.call_count, 1)

    @mock.patch("dds2val.src.ddsfeeder.time.sleep")
    def test_run_sleeps_when_not_connected(self, mock_sleep):
        ddsfeeder = self.create_dds_feeder()

        def stop_ddsfeeder(*unused_sleep_arguments):
            ddsfeeder.stop()

        # Make sure main loop is executed only once
        mock_sleep.side_effect = stop_ddsfeeder

        ddsfeeder._run()

        # Check if sleep was called
        self.assertEqual(mock_sleep.call_count, 1)

    @mock.patch("dds2val.src.ddsfeeder.Ddsfeeder._subscribe")
    @mock.patch("dds2val.src.ddsfeeder.time.sleep")
    @mock.patch("dds2val.src.ddsfeeder.Ddsfeeder._should_shutdown")
    def test_run_does_nothing_when_connected_and_subscribed(
        self, mock_should_shutdown, mock_sleep, mock_subscribe
    ):
        ddsfeeder = self.create_dds_feeder()

        def stop_after_3_iterations():
            return mock_should_shutdown.call_count > 3

        mock_should_shutdown.side_effect = stop_after_3_iterations

        ddsfeeder._connected = True
        ddsfeeder._subscribed = True

        ddsfeeder._run()

        # Check if sleep and subscribe are never called
        self.assertEqual(mock_sleep.call_count, 0)
        self.assertEqual(mock_subscribe.call_count, 0)

    def test_stop(self):
        ddsfeeder = self.create_dds_feeder()
        ddsfeeder.stop()
        self.assertTrue(ddsfeeder._should_shutdown())

    @mock.patch("dds2val.src.databroker.Provider")
    @mock.patch("dds2val.src.ddsfeeder.getattr")
    def test_on_data_available(self, mock_getattr, mock_provider):
        # Check behaviour when dds type class is not valid

        listener = DdsListener(
            provider=mock_provider,
            mapper=Vss2DdsMapper(self.mappingfile),
        )

        reader = mock.Mock(spec=DataReader)
        reader.take_next.return_value = 10
        reader.topic.get_name.return_value = "Cabin_Lights_Ambient_Light"

        mock_getattr.side_effect = AttributeError()

        listener.on_data_available(reader=reader)
        mock_provider.assert_not_called()


if __name__ == "__main__":
    unittest.main()
