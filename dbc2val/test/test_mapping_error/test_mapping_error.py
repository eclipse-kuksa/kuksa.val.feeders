#!/usr/bin/python3

########################################################################
# Copyright (c) 2023 Robert Bosch GmbH
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0
########################################################################

from dbcfeederlib import dbc2vssmapper
from dbcfeederlib import dbcparser
import os
import pytest
import logging

test_path = os.path.dirname(os.path.abspath(__file__))


def test_unknown_transform(caplog: pytest.LogCaptureFixture):

    mapping_path = test_path + "/test_unknown_transform.json"
    parser = dbcparser.DBCParser(test_path + "/../../Model3CAN.dbc")

    with pytest.raises(SystemExit) as excinfo:
        dbc2vssmapper.Mapper(mapping_path, parser)
    assert excinfo.value.code == -1
    assert ("dbcfeederlib.dbc2vssmapper", logging.ERROR, "Unsupported transform for A.B") in caplog.record_tuples


def test_mapper_fails_for_duplicate_signal_definition():

    mapping_path = test_path + "/mapping_for_ambiguous_signal.json"
    parser = dbcparser.DBCParser(test_path + "/../test_dbc/duplicate_signal_name.kcd")

    with pytest.raises(SystemExit) as excinfo:
        dbc2vssmapper.Mapper(mapping_path, parser, fail_on_duplicate_signal_definitions=True)
    assert excinfo.value.code == -1


def test_mapper_ignores_unused_duplicate_signal_definition():

    mapping_path = test_path + "/mapping_for_unused_ambiguous_signal.json"
    parser = dbcparser.DBCParser(test_path + "/../test_dbc/duplicate_signal_name.kcd")
    mapper = dbc2vssmapper.Mapper(mapping_path, parser, fail_on_duplicate_signal_definitions=True)
    affected_signal_names = mapper.handle_update("A.B", 15)
    assert len(affected_signal_names) == 1
    assert "SignalTwo" in affected_signal_names
