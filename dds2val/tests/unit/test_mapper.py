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

from ddsproviderlib.vss2ddsmapper import Vss2DdsMapper


class TestMapper(unittest.TestCase):
    """Corner cases for mapper"""

    def setUp(self):
        mappingfile = (
            Path(__file__).parent.parent.parent
            / "mapping.yml"
        )
        self.mapper = Vss2DdsMapper(str(mappingfile))

    def test_contains_method(self):
        assert "Vehicle.Cabin.Lights.AmbientLight" in self.mapper

    def test_getitem_method(self):
        assert self.mapper["Vehicle.Cabin.Lights.AmbientLight"] is not None
