#! /usr/bin/env python3

########################################################################
# Copyright (c) 2023 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License 2.0 which is available at
# http://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
########################################################################
'''A recording writing signals from an instance of the KUKSA.val databroker
 to a CSV-file'''
import argparse
import asyncio
import csv
import logging
import time

from kuksa_client.grpc.aio import VSSClient
from kuksa_client.grpc import VSSClientError
from kuksa_client.grpc import View
from kuksa_client.grpc import SubscribeEntry

from kuksa_client.grpc import Field


def init_argparse() -> argparse.ArgumentParser:
    '''This inits the argument parser for the CSV-recorder.'''
    parser = argparse.ArgumentParser(
        usage="-a [BROKER ADDRESS] -p [BROKER PORT] -f [FILE] -s [SIGNALS] -l [LOGGING LEVEL]",
        description="This provider writes the content of a csv file to a KUKSA.val databroker")
    parser.add_argument("-a", "--address", default="127.0.0.1", help="This indicates the address"
                        " of the KUKSA.val databroker to connect to."
                        " The default value is 127.0.0.1")
    parser.add_argument("-p", "--port", default="55555", help="This indicates the port"
                        " of the KUKSA.val databroker to connect to."
                        " The default value is 55555", type=int)
    parser.add_argument("-f", "--file", default="signalsOut.csv", help="This indicates the csv file"
                        " to write the signals to."
                        " The default value is signals.csv.")
    parser.add_argument("-s", "--signals", help="A list of signals to"
                        " record", nargs='+', required=True)
    parser.add_argument("-l", "--log", default="INFO", help="This sets the logging level."
                        " The default value is WARNING.",
                        choices={"INFO", "DEBUG", "WARNING", "ERROR", "CRITICAL"})
    return parser


async def main():
    '''entrypoint to the CSV-recorder'''
    args = init_argparse().parse_args()
    numeric_value = getattr(logging, args.log.upper(), None)
    if isinstance(numeric_value, int):
        logging.basicConfig(encoding='utf-8', level=numeric_value)
    try:
        async with VSSClient(args.address, args.port) as client:
            csvfile = open(args.file, 'w', newline='', encoding="utf-8")
            signalwriter = csv.DictWriter(csvfile, ['field', 'signal', 'value', 'delay'])
            signalwriter.writeheader()
            previous_time = time.time()
            initial_value = True
            entries = []
            for signal in args.signals:
                entries.append(SubscribeEntry(signal,
                                              View.FIELDS,
                                              (Field.VALUE, Field.ACTUATOR_TARGET)))
            async for updates in client.subscribe(entries=entries):
                if initial_value:
                    time_gap = 0.0
                    initial_value = False
                else:
                    current_time = time.time()
                    time_gap = current_time - previous_time
                    previous_time = current_time
                for update in updates:
                    entry = update.entry
                    if entry.value is not None:
                        signalwriter.writerow({'field': 'current',
                                               'signal': entry.path,
                                               'value': entry.value.value,
                                               'delay': time_gap})
                    if entry.actuator_target is not None:
                        signalwriter.writerow({'field': 'target',
                                               'signal': entry.path,
                                               'value': entry.actuator_target.value,
                                               'delay': time_gap})
    except VSSClientError as error:
        logging.error("There was a problem in the interaction"
                      " with the KUKSA.val databroker at %s:%s: %s ",
                      args.address, args.port, str(error))

asyncio.run(main())
