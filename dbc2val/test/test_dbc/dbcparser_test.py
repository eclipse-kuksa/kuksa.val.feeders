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

from dbcfeederlib import dbcparser
import os

# read config only once
test_path = os.path.dirname(os.path.abspath(__file__))
def_dbc = test_path + "/../../Model3CAN.dbc"


def test_default_dbc():

    parser = dbcparser.DBCParser(def_dbc)
    msg_list = parser.get_messages_for_signal('SteeringAngle129')
    assert len(msg_list) == 1
    assert msg_list[0].frame_id == 297
    msg_list = parser.get_messages_for_signal('DI_uiSpeed')
    assert len(msg_list) == 1
    assert msg_list[0].frame_id == 599

    signals = [signal.name for signal in parser.get_signals_by_frame_id(599)]
    assert "DI_speedChecksum" in signals
    assert "DI_speedCounter" in signals
    assert "DI_uiSpeed" in signals
    assert "DI_uiSpeedHighSpeed" in signals
    assert "DI_uiSpeedUnits" in signals
    assert "DI_vehicleSpeed" in signals


def test_split_dbc():
    """
    This verifies that we can read a DBC database split across multiple files.
    Difference compared to default is that 'SteeringAngle129' has been moved to test1_2.dbc
    """
    parser = dbcparser.DBCParser(test_path + "/test1_1.dbc," + test_path + "/test1_2.dbc")
    msg_list = parser.get_messages_for_signal('SteeringAngle129')
    assert len(msg_list) == 1
    assert msg_list[0].frame_id == 297
    msg_list = parser.get_messages_for_signal('DI_uiSpeed')
    assert len(msg_list) == 1
    assert msg_list[0].frame_id == 599


def test_duplicated_dbc():
    """
    Load original DBC multiple times
    """
    parser = dbcparser.DBCParser(def_dbc + ',' + def_dbc)
    msg_list = parser.get_messages_for_signal('SteeringAngle129')
    assert len(msg_list) == 1
    assert msg_list[0].frame_id == 297
    msg_list = parser.get_messages_for_signal('DI_uiSpeed')
    assert len(msg_list) == 1
    assert msg_list[0].frame_id == 599


def test_single_kcd():

    parser = dbcparser.DBCParser(test_path + "/test1_1.kcd")
    msg_list = parser.get_messages_for_signal('DI_bmsRequestInterfaceVersion')
    assert len(msg_list) == 1
    assert msg_list[0].frame_id == 0x16

    signals = [signal.name for signal in parser.get_signals_by_frame_id(0x16)]
    assert "DI_bmsOpenContactorsRequest" in signals
    assert "DI_bmsRequestInterfaceVersion" in signals


def test_split_kcd():
    """
    This verifies that we can read multiple KCD files.
    """
    parser = dbcparser.DBCParser(test_path + "/test1_1.kcd," + test_path + "/test1_2.kcd")
    msg_list = parser.get_messages_for_signal('DI_bmsRequestInterfaceVersion')
    assert len(msg_list) == 1
    assert msg_list[0].frame_id == 0x16
    msg_list = parser.get_messages_for_signal('SteeringAngle129')
    assert len(msg_list) == 1
    assert msg_list[0].frame_id == 0x129


def test_mixed_file_types():
    """
    This verifies that we can read files of different type.
    """
    parser = dbcparser.DBCParser(test_path + "/test1_1.kcd," + test_path + "/test1_2.dbc")
    msg_list = parser.get_messages_for_signal('DI_bmsRequestInterfaceVersion')
    assert len(msg_list) == 1
    assert msg_list[0].frame_id == 0x16
    msg_list = parser.get_messages_for_signal('SteeringAngle129')
    assert len(msg_list) == 1
    assert msg_list[0].frame_id == 0x129


def test_duplicate_signal_name_dbc():
    # GIVEN a database with two messages defining a signal of name SignalOne
    parser = dbcparser.DBCParser(test_path + "/duplicate_signal_name.kcd")

    # WHEN looing up the two messages by name
    msg1 = parser._db.get_message_by_name('First')
    msg2 = parser._db.get_message_by_name('Second')

    # THEN they have different frame IDs
    assert msg1.frame_id != msg2.frame_id

    # AND they both have a signal of name SignalOne
    assert msg1.get_signal_by_name('SignalOne') is not None
    assert msg2.get_signal_by_name('SignalOne') is not None

    # BUT WHEN looking up the CAN id for SignalOne
    # THEN the IDs of both messages defining the signal are found
    msg_list = parser.get_messages_for_signal('SignalOne')
    assert len(msg_list) == 2
    assert msg1 in msg_list
    assert msg2 in msg_list
