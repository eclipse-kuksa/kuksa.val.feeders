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
from unittest import mock

import kuksa_client
from kuksa_client.grpc import DataType, Metadata, VSSClientError

from ddsproviderlib.databroker import Provider


class TestFeederBroker(unittest.TestCase):
    def setUp(self) -> None:
        return super().setUp()

    @mock.patch("kuksa_client.grpc")
    def test_register(self, mock_client):
        # Check Behaviour for signals not registered in broker
        mock_client.get_metadata.return_value = {}
        updates = {
            "Vehicle.Cabin.Light.PerceivedAmbientLight": Metadata(
                data_type=DataType.INT8, description="Ambient light value"
            )
        }
        provider = Provider(mock_client)

        self.assertTrue(
            provider.register(
                "Vehicle.Cabin.Light.PerceivedAmbientLight", "INT8", "Ambient light value"
            )
        )
        mock_client.set_metadata.assert_called_with(updates=updates, metadata=None)

    @mock.patch(
        "kuksa_client.grpc.VSSClient.get_metadata"
    )
    def test_register_exception_get_metadata(self, mock_get):
        # Check Behaviour when exception is thrown while connecting to broker
        mock_get.side_effect = VSSClientError(error={"error": "some error"}, errors=[])

        provider = Provider(
            vss_client=kuksa_client.grpc.VSSClient(host="127.0.0.1", port="55555")
        )
        self.assertFalse(
            provider.register(
                "Vehicle.Cabin.Light.PerceivedAmbientLight", "INT8", "Ambient light value"
            )
        )

    @mock.patch(
        "kuksa_client.grpc.VSSClient.get_metadata"
    )
    @mock.patch(
        "kuksa_client.grpc.VSSClient.set_metadata"
    )
    def test_register_exception_set_metadata(self, mock_set, mock_get):
        # Check Behaviour when exception is thrown while connecting to broker
        mock_set.side_effect = VSSClientError(error={"error": "some error"}, errors=[])
        mock_get.return_value = []
        provider = Provider(
            vss_client=kuksa_client.grpc.VSSClient(host="127.0.0.1", port="55555")
        )
        self.assertFalse(
            provider.register(
                "Vehicle.Cabin.Light.PerceivedAmbientLight", "INT8", "Ambient light value"
            )
        )

    @mock.patch(
        "kuksa_client.grpc.VSSClient.set"
    )
    def test_update_datapoint_set_exception(self, mock_set):
        # Check Behaviour when exception is thrown while updating data point
        mock_set.side_effect = VSSClientError(error={"error": "some error"}, errors=[])
        provider = Provider(
            vss_client=kuksa_client.grpc.VSSClient(host="127.0.0.1", port="55555")
        )
        provider._name_to_type = {"Vehicle.Cabin.Light.PerceivedAmbientLight": "UINT8"}
        self.assertFalse(
            provider.update_datapoint("Vehicle.Cabin.Light.PerceivedAmbientLight", 20)
        )
