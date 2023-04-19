import asyncio
import csv
import argparse
import logging

from kuksa_client.grpc import Datapoint
from kuksa_client.grpc import DataEntry
from kuksa_client.grpc import EntryUpdate
from kuksa_client.grpc import Field
from kuksa_client.grpc import VSSClientError
from kuksa_client.grpc.aio import VSSClient


def init_argparse() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        usage="-a [BROKER ADDRESS] -p [BROKER PORT] -f [FILE]",
        description="This provider writes the content of a csv file to a kuksa.val databroker",
    )
    parser.add_argument("-a", "--address", default="127.0.0.1", help="This indicates the address" +
                        " of the kuksa.val databroker to connect to. The default value is 127.0.0.1")
    parser.add_argument("-p", "--port", default="55555", help="This indicates the port" +
                        " of the kuksa.val databroker to connect to. The default value is 5555", type=int)
    parser.add_argument("-f", "--file", default="signals.csv", help="This indicates the csv file" +
                        " containing the signals to update in the kuksa.val databroker." +
                        " The default value is signals.csv.")
    parser.add_argument("-i", "--infinite", action=argparse.BooleanOptionalAction,
                        help="If the flag is set, the provider loops" +
                        "the file until stopped, otherwise the file gets processed once.")
    parser.add_argument("-l", "--log", default="INFO", help="This sets the logging level." +
                        " The default value is WARNING.",
                        choices={"INFO", "DEBUG", "WARNING", "ERROR", "CRITICAL"})
    return parser


async def main():
    parser = init_argparse()
    args = parser.parse_args()
    numeric_value = getattr(logging, args.log.upper(), None)
    if isinstance(numeric_value, int):
        logging.basicConfig(encoding='utf-8', level=numeric_value)
    try:
        async with VSSClient(args.address, args.port) as client:
            csvfile = open(args.file, newline='')
            signal_reader = csv.DictReader(csvfile, delimiter=',', quotechar='|', skipinitialspace=True)
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
        logging.error("Could not connect to the kuksa.val databroker at %s:%s. " +
                      "Make sure to set the correct connection details using --address and --port" +
                      "and that the kuksa.val databroker is running.", args.address, args.port)


async def process_rows(client, rows):
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
        except Exception as ex:
            logging.error("Error while updating %s\n%s", row['signal'], ex)
        try:
            await asyncio.sleep(float(row['delay']))
        except Exception:
            logging.error("Error while waiting for '%s' seconds after updating %s.\n" +
                          "Make sure to only use numbers for the delay value.", row['sleep'], row['delay'])
asyncio.run(main())
