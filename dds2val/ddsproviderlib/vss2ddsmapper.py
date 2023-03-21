#!/usr/bin/python3

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

import yaml
from py_expression_eval import Parser

log = logging.getLogger(__name__)


def math(spec, value):
    """Apply mathematical transformations to signals."""
    return Parser().parse(spec).evaluate({"x": value})


def formula(spec, value):
    """Apply value = input * nominator / denominator + offset."""
    nominator = int(spec.get("nominator", 1))
    denominator = int(spec.get("denominator", 1))
    offset = int(spec.get("offset", 0))
    return ((value * nominator) / denominator) + offset


class Vss2DdsMapper:
    """Maps VSS data points to DDS messages."""

    def __init__(self, input_file):
        with open(input_file, "r", encoding="UTF-8") as file:
            self.mapping = yaml.full_load(file)

        self.dds2vss_dict = self._createdict(self.mapping)

    def map(self):
        return self.mapping.items()

    def transform(self, ddstopic, vsssignal, value):
        """Apply transforms on dds message onto VSS topic, if defined."""
        # Get the transform from
        vsssignals = self.dds2vss_dict[ddstopic]["vsssignals"]

        for entry in vsssignals:
            if entry["vsssignal"] == vsssignal:
                # found signal matching entry
                value = formula(entry["transform"]["formula"], value)

        return value

    def _createdict(self, mapping):
        """Make a dict key= dds topic name , value ={vsssignals,typename}.

        where vsssignals = list of all vss points in this dds topic
        typename is the dataclass name

        Ex:
        Nav_Sat_Fix:
            vsssignals:
            [
                {
                    vsssignal: latitude
                    element: latitude
                    transform: dict directly from the mapping
                },
                {
                    vsssignal: altitude
                    element: altitude
                    transform: dict directly from the mapping
                },
                ...
            ],
            typename: sensor_msgs.msg.NavSatFix

        ...
        """
        dds2vss: dict[dict, dict] = {}

        for entry in mapping:
            ddstopicname = next(iter(mapping[entry]["source"]))
            if ddstopicname not in dds2vss:
                # Create a sub dict for each dds topic
                dds2vss[ddstopicname] = {}
            if "vsssignals" not in dds2vss[ddstopicname]:
                # Create vsssignals sub dict for the first vss point found
                dds2vss[ddstopicname].update({"vsssignals": []})

            dds2vss[ddstopicname]["typename"] = mapping[entry]["source"][
                ddstopicname
            ].get("typename")

            vssinfo = {
                "vsssignal": entry,
                "element": mapping[entry]["source"][ddstopicname].get("element", ""),
                "transform": mapping[entry]["source"][ddstopicname].get(
                    "transform", ""
                ),
            }

            dds2vss[ddstopicname]["vsssignals"].append(vssinfo)

        return dds2vss

    def __contains__(self, key):
        return key in self.mapping.keys()

    def __getitem__(self, item):
        return self.mapping[item]
