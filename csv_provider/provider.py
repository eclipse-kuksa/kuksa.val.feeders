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
'''A provider accepting VSS-signals from a CSV-file
 to write these signals into an Kuksa.val data broker'''

import asyncio
import csv
import argparse
import logging
import os
from pathlib import Path

from kuksa_client.grpc import Datapoint
from kuksa_client.grpc import DataEntry
from kuksa_client.grpc import EntryUpdate
from kuksa_client.grpc import Field
from kuksa_client.grpc import VSSClientError
from kuksa_client.grpc.aio import VSSClient


def init_argparse() -> argparse.ArgumentParser:
    '''This inits the argument parser for the CSV-provider.'''
    parser = argparse.ArgumentParser(
        usage="-a [BROKER ADDRESS] -p [BROKER PORT] -f [FILE]",
        description="This provider writes the content of a csv file to a kuksa.val databroker",
    )
    environment = os.environ
    parser.add_argument("-a", "--address", default=environment.get("KUKSA_DATA_BROKER_ADDR",
                                                                   "127.0.0.1"),
                        help="This indicates the address of the kuksa.val databroker to connect to."
                        " The default value is 127.0.0.1")
    parser.add_argument("-p", "--port", default=environment.get('KUKSA_DATA_BROKER_PORT', "55555"),
                        help="This indicates the port of the kuksa.val databroker to connect to."
                        " The default value is 55555", type=int)
    parser.add_argument("-f", "--file", default=environment.get("PROVIDER_SIGNALS_FILE",
                                                                "signals.csv"),
                        help="This indicates the csv file containing the signals to update in"
                        " the kuksa.val databroker. The default value is signals.csv.")
    parser.add_argument("-i", "--infinite", default=environment.get("PROVIDER_INFINITE"),
                        action=argparse.BooleanOptionalAction,
                        help="If the flag is set, the provider loops"
                        "the file until stopped, otherwise the file gets processed once.")
    parser.add_argument("-l", "--log", default=environment.get("PROVIDER_LOG_LEVEL", "INFO"),
                        help="This sets the logging level. The default value is WARNING.",
                        choices={"INFO", "DEBUG", "WARNING", "ERROR", "CRITICAL"})
    parser.add_argument("--cacertificate",
                        help="Specify the path to your CA.pem. If used provider will connect using TLS",
                        nargs='?', default=None)
    parser.add_argument("--tls-server-name",
                        help="TLS server name, may be needed if addressing a server by IP-name",
                        nargs='?', default=None)
    return parser


async def main():
    '''the main function as entry point for the CSV-provider'''
    parser = init_argparse()
    args = parser.parse_args()
    numeric_value = getattr(logging, args.log.upper(), None)
    if args.cacertificate:
        root_path = Path(args.cacertificate)
    else:
        root_path = None
    if isinstance(numeric_value, int):
        logging.basicConfig(encoding='utf-8', level=numeric_value)
    try:
        async with VSSClient(args.address, args.port, root_certificates=root_path,
                             tls_server_name=args.tls_server_name) as client:
            csvfile = open(args.file, newline='', encoding="utf-8")
            signal_reader = csv.DictReader(csvfile,
                                           delimiter=',',
                                           quotechar='|',
                                           skipinitialspace=True)
            logging.info("Starting to apply the signals read from %s.", str(csvfile.name))
            if args.infinite:
                backup = list(signal_reader)
                while True:
                    rows = backup
                    backup = list(rows)
                    await process_rows(client, rows)
            else:
                await process_rows(client, signal_reader)
    except VSSClientError:
        logging.error("Could not connect to the kuksa.val databroker at %s:%s."
                      " Make sure to set the correct connection details using --address and --port"
                      " and that the kuksa.val databroker is running.", args.address, args.port)


async def process_rows(client, rows):
    '''Processes a single row from the CSV-file and write the
     recorded signal to the data broker through the client.'''
    for row in rows:
        entry = DataEntry(
            row['signal'],
            value=Datapoint(value=row['value']),
            )
        if row['field'] == "current":
            updates = (EntryUpdate(entry, (Field.VALUE,)),)
            logging.info("Update current value of %s to %s", row['signal'], row['value'])
        elif row['field'] == "target":
            updates = (EntryUpdate(entry, (Field.ACTUATOR_TARGET,)),)
            logging.info("Update target value of %s to %s", row['signal'], row['value'])
        else:
            updates = []
        try:
            await client.set(updates=updates)
        except VSSClientError as ex:
            logging.error("Error while updating %s\n%s", row['signal'], ex)
        try:
            await asyncio.sleep(delay=float(row['delay']))
        except ValueError:
            logging.error("Error while waiting for %s seconds after updating %s to %s."
                          " Make sure to only use numbers for the delay value.",
                          row['delay'], row['signal'], row['value'])

asyncio.run(main())
