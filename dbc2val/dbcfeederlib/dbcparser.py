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

import logging
import sys
import os
from typing import cast, Dict, List, Tuple
import cantools.database

log = logging.getLogger(__name__)


class DBCParser:
    def __init__(self, dbcfile: str, use_strict_parsing: bool = True):
        """Create a parser from definitions in a DBC file."""

        first = True
        found_names = set()
        for name in dbcfile.split(","):
            filename = name.strip()
            if filename in found_names:
                log.warning("The DBC file %s has already been read, ignoring it!", filename)
                continue
            found_names.add(filename)
            if first:
                log.info("Reading definitions from DBC file %s", filename)
                database = cantools.database.load_file(filename, strict=use_strict_parsing)
                # load_file can return multiple types of databases, make sure we have CAN database
                if isinstance(database, cantools.database.can.database.Database):
                    self._db = cast(cantools.database.can.database.Database, database)
                    first = False
                else:
                    log.error("File %s is not a CAN database, likely a diagnostics database", filename)
                    sys.exit(-1)
            else:
                log.info("Adding definitions from DBC file %s", filename)
                self._add_db_file(filename)

        # Init some dictionaries to speed up search
        self._signal_to_message_definitions = self._populate_signal_to_message_map()

    def _populate_signal_to_message_map(self) -> Dict[str, List[cantools.database.Message]]:

        signal_to_message_defs: Dict[str, List[cantools.database.Message]] = {}

        for msg_definition in self._db.messages:
            for signal in msg_definition.signals:
                if signal.name in signal_to_message_defs:
                    signal_to_message_defs[signal.name].append(msg_definition)
                else:
                    signal_to_message_defs[signal.name] = list({msg_definition})

        if log.isEnabledFor(logging.WARNING):
            for (sig_name, messages) in signal_to_message_defs.items():
                if len(messages) > 1:
                    log.warning(
                        "Signal name %s is being used in multiple messages (%s).",
                        sig_name, ', '.join([msg_def.name for msg_def in messages])
                    )
            log.warning(
                """Make sure that signals have the same semantics in all messages where they are used
                to prevent unexpected behaviour when mapping VSS datapoints to these signals."""
            )

        return signal_to_message_defs

    def _determine_db_format_and_encoding(self, filename) -> Tuple[str, str]:
        db_format = os.path.splitext(filename)[1][1:].lower()

        try:
            encoding = {
                'dbc': 'cp1252',
                'sym': 'cp1252'
            }[db_format]
        except KeyError:
            encoding = 'utf-8'

        return db_format, encoding

    def _add_db_file(self, filename: str):
        db_format, encoding = self._determine_db_format_and_encoding(filename)
        if db_format == "arxml":
            self._db.add_arxml_file(filename, encoding)
        elif db_format == "dbc":
            self._db.add_dbc_file(filename, encoding)
        elif db_format == "kcd":
            self._db.add_kcd_file(filename, encoding)
        elif db_format == "sym":
            self._db.add_sym_file(filename, encoding)
        else:
            log.warning("Cannot read CAN message definitions from file using unsupported format: %s", db_format)

    def get_messages_for_signal(self, sig_to_find: str) -> List[cantools.database.Message]:
        """Get all CAN message definitions that use a given CAN signal name."""
        if sig_to_find in self._signal_to_message_definitions:
            return self._signal_to_message_definitions[sig_to_find]

        log.warning("Signal %s not found in CAN message database", sig_to_find)
        empty_list = []
        self._signal_to_message_definitions[sig_to_find] = empty_list
        return empty_list

    def get_message_by_frame_id(self, frame_id: int) -> cantools.database.Message:
        """Get the CAN message definition for a given CAN frame ID."""
        return self._db.get_message_by_frame_id(frame_id)

    def get_signals_by_frame_id(self, frame_id: int) -> List[cantools.database.Signal]:
        """Get the signals of the CAN message definition for a given CAN frame ID."""
        try:
            return self.get_message_by_frame_id(frame_id).signals
        except Exception:
            log.warning("CAN id %s not found in CAN message database", frame_id)
            return []
