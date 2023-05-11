#! /usr/bin/env python3

########################################################################
# Copyright (c) 2020-2023 Contributors to the Eclipse Foundation
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

# This provider gets data from a simple log file which contains lines
# formatted
# lat,lon
# All other values will be reported as 0

import threading
import configparser
import os
import sys
import json
import signal
import time
import logging
from gpsdclient import GPSDClient
import argparse
from kuksa_client import KuksaClientThread

scriptDir = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(scriptDir, "../../"))


log = logging.getLogger("gpsfeeder")


def init_logging(loglevel):
    # create console handler and set level to debug
    console_logger = logging.StreamHandler()
    console_logger.setLevel(logging.DEBUG)

    # create formatter
    if sys.stdout.isatty():
        formatter = ColorFormatter()
    else:
        formatter = logging.Formatter(
            fmt="%(asctime)s %(levelname)s %(name)s: %(message)s"
        )

    # add formatter to console_logger
    console_logger.setFormatter(formatter)

    # add console_logger as a global handler
    root_logger = logging.getLogger()
    root_logger.setLevel(loglevel)
    root_logger.addHandler(console_logger)


class ColorFormatter(logging.Formatter):
    FORMAT = "{time} {{loglevel}} {logger} {msg}".format(
        time="\x1b[2m%(asctime)s\x1b[0m",  # grey
        logger="\x1b[2m%(name)s:\x1b[0m",  # grey
        msg="%(message)s",
    )
    FORMATS = {
        logging.DEBUG: FORMAT.format(loglevel="\x1b[34mDEBUG\x1b[0m"),  # blue
        logging.INFO: FORMAT.format(loglevel="\x1b[32mINFO\x1b[0m"),  # green
        logging.WARNING: FORMAT.format(loglevel="\x1b[33mWARNING\x1b[0m"),  # yellow
        logging.ERROR: FORMAT.format(loglevel="\x1b[31mERROR\x1b[0m"),  # red
        logging.CRITICAL: FORMAT.format(loglevel="\x1b[31mCRITICAL\x1b[0m"),  # red
    }

    def format(self, record):
        log_fmt = self.FORMATS.get(record.levelno)
        formatter = logging.Formatter(log_fmt)
        return formatter.format(record)


def parse_env_log(env_log, default=logging.INFO):
    def parse_level(level, default=default):
        if type(level) is str:
            if level.lower() in [
                "debug",
                "info",
                "warn",
                "warning",
                "error",
                "critical",
            ]:
                return level.upper()
            else:
                raise Exception(f"could not parse '{level}' as a log level")
        return default

    loglevels = {}

    if env_log is not None:
        log_specs = env_log.split(",")
        for log_spec in log_specs:
            spec_parts = log_spec.split("=")
            if len(spec_parts) == 1:
                # This is a root level spec
                if "root" in loglevels:
                    raise Exception("multiple root loglevels specified")
                else:
                    loglevels["root"] = parse_level(spec_parts[0])
            if len(spec_parts) == 2:
                logger = spec_parts[0]
                level = spec_parts[1]
                loglevels[logger] = parse_level(level)

    if "root" not in loglevels:
        loglevels["root"] = default

    return loglevels


class Kuksa_Client():

    # Constructor
    def __init__(self, config):
        log.info("Init kuksa client...")
        if "kuksa_val" not in config:
            log.error("kuksa_val section missing from configuration, exiting")
            sys.exit(-1)
        provider_config = config['kuksa_val']
        self.client = KuksaClientThread(provider_config)
        self.client.start()
        token_string = str(provider_config.get('token_or_tokenfile'))
        if token_string != "":
            log.info(f"Token information provided is: {token_string}")
            self.client.authorize(token_string)
        else:
            log.info("No token information provided, "
                     "subsequent errors expected if Server/Databroker requires authentication!")
        connected = self.client.checkConnection()
        log.info(f"Connection status is: {connected}")
        self.messages_sent = 0
        self.last_sent_log_entry = 0

    def shutdown(self):
        self.client.stop()

    def setData(self, data):
        log.debug(f"Update {data}")
        for k, v in data.items():
            if v is not None:
                tmp_text = self.client.setValue(k, str(v))
                log.debug(f"Got setValue response:{tmp_text}")
                if (tmp_text == "OK"):
                    # Databroker returns OK on successful calls, it only returns JSON in case of errors
                    resp = {}
                else:
                    try:
                        resp = json.loads(tmp_text)
                    except json.decoder.JSONDecodeError:
                        log.error(f"Unexpected response from Server/Databroker: {tmp_text}", exc_info=True)
                        resp = {"error": tmp_text}
                if "error" in resp:
                    log.error(f"Error sending signal: {resp['error']}")
                else:
                    self.messages_sent += 1
                    if self.messages_sent >= (2 * self.last_sent_log_entry):
                        log.info(f"Number of VSS messages sent so far: {self.messages_sent}")
                        self.last_sent_log_entry = self.messages_sent


class GPSDClientThread(threading.Thread):
    def __init__(self, config, consumer):
        super(GPSDClientThread, self).__init__()
        log.info("Init gpsd client...")
        if "gpsd" not in config:
            log.error("gpsd section missing from configuration, exiting")
            sys.exit(-1)

        self.consumer = consumer
        provider_config = config['gpsd']
        self.gpsd_host = provider_config.get('host', '127.0.0.1')
        self.gpsd_port = provider_config.get('port', '2947')
        self.interval = provider_config.getint('interval', 1)

        log.info("Trying to connect gpsd at "+str(self.gpsd_host)+" port "+str(self.gpsd_port))
        self.client = GPSDClient(host=self.gpsd_host, port=self.gpsd_port)

        self.running = True

    def run(self):
        log.info("gpsd receive loop started")
        try:
            for result in self.client.dict_stream(filter=["TPV"]):
                if self.running:
                    log.debug("Data received")
                    collecteddata = {}
                    collecteddata['Vehicle.CurrentLocation.Latitude'] = result.get('lat')
                    collecteddata['Vehicle.CurrentLocation.Longitude'] = result.get('lon')
                    collecteddata['Vehicle.CurrentLocation.Altitude'] = result.get('alt')
                    collecteddata['Vehicle.Speed'] = result.get('speed')
                    collecteddata['Vehicle.CurrentLocation.Timestamp'] = result.get('time')
                    collecteddata['Vehicle.CurrentLocation.Heading'] = result.get('track')
                    collecteddata['Vehicle.CurrentLocation.HorizontalAccuracy'] = result.get('eph')
                    collecteddata['Vehicle.CurrentLocation.VerticalAccuracy'] = result.get('epv')

                    self.consumer.setData(collecteddata)
                    time.sleep(self.interval)
                else:
                    log.info("Exiting")
                    break
        except Exception:
            log.error("Exception listening to gpsd", exc_info=True)

    def shutdown(self):
        self.running = False
        self.consumer.shutdown()
        log.info("KUKSA client shutdown")
        self.client.close()
        log.info("GPSD client shutdown")
        self.join(1)
        if not self.is_alive():
            log.info("Shutdown completed")
        else:
            log.info("Shutdown join timed out!")


if __name__ == "__main__":

    # Example
    # Set log level to debug
    #   LOG_LEVEL=debug ./dbcfeeder.py
    #
    # Set log level to INFO, but for kuksa_client set it to DEBUG
    #   LOG_LEVEL=info,kuksa_client=debug ./dbcfeeder.py
    #
    # Other available loggers:
    #   gpsfeeder (main gpsfeeder file)
    #   kuksa_client (If you want to get additional information from kuksa-client python library)
    #

    loglevels = parse_env_log(os.environ.get("LOG_LEVEL"))

    # set root loglevel etc
    init_logging(loglevels["root"])

    # set loglevels for other loggers
    for logger, level in loglevels.items():
        if logger != "root":
            logging.getLogger(logger).setLevel(level)

    manual_config = argparse.ArgumentParser()
    manual_config.add_argument("--host",
                               help="Specify the host where too look for KUKSA.val server/databroker; "
                                    "default: 127.0.0.1",
                               nargs='?', default="127.0.0.1")
    manual_config.add_argument("--port",
                               help="Specify the port where too look for KUKSA.val server/databroker; default: 8090",
                               nargs='?', default="8090")
    manual_config.add_argument("--protocol",
                               help="If you want to connect to KUKSA.val server specify ws. "
                                    "If you want to connect to KUKSA.val databroker specify grpc; default: ws",
                                    nargs='?', default="ws")
    manual_config.add_argument("--insecure",
                               help="For KUKSA.val server specify False, "
                                    "for KUKSA.val databroker there is currently no security so specify True; "
                                    "default: False",
                               nargs='?', default="False")
    manual_config.add_argument("--certificate",
                               help="Specify the path to your Client.pem file; default: Client.pem",
                               nargs='?', default="Client.pem")
    manual_config.add_argument("--cacertificate",
                               help="Specify the path to your CA.pem; default: CA.pem",
                               nargs='?', default="CA.pem")
    manual_config.add_argument("--token",
                               help="Specify the JWT token string or the path to your JWT token; default: "
                                    "authentication information not specified",
                               nargs='?', default="")
    manual_config.add_argument("--file",
                               help="Specify the path to your config file; by default not defined",
                               nargs='?', default="")
    manual_config.add_argument("--gpsd_host", help="Specify the host for gpsd to start on; default: 127.0.0.1",
                               nargs='?', default="127.0.0.1")
    manual_config.add_argument("--gpsd_port", help="Specify the port for gpsd to start on; default: 2948",
                               nargs='?', default="2948")
    manual_config.add_argument("--interval", help="Specify the interval time for feeding gps data; default: 1",
                               nargs='?', default="1")
    args = manual_config.parse_args()
    log.debug(f"Command line args: {args}")
    if os.path.isfile(args.file):
        configfile = args.file
        log.info("# Using config from: {}".format(configfile))
    else:
        config_object = configparser.ConfigParser()
        log.info("No configuration file found. Using default values.")
        config_object["kuksa_val"] = {
            "host": args.host,
            "port": args.port,
            "protocol": args.protocol,
            "insecure": args.insecure,
            "certificate": args.certificate,
            "cacertificate": args.cacertificate,
            "token_or_tokenfile": args.token,
            "file": args.file,
        }
        config_object["gpsd"] = {
            "interval": args.interval,
            "host": args.gpsd_host,
            "port": args.gpsd_port,
        }
        log.debug(f"Writing config: {config_object}")
        with open('config.ini', 'w') as conf:
            config_object.write(conf)
        configfile = "config.ini"

    config = configparser.ConfigParser()
    config.read(configfile)

    gpsd_client = GPSDClientThread(config, Kuksa_Client(config))
    log.info("Using mapping")
    gpsd_client.start()

    def terminationSignalreceived(signalNumber, frame):
        log.info("Received termination signal. Shutting down")
        gpsd_client.shutdown()
        os._exit(1)

    signal.signal(signal.SIGINT, terminationSignalreceived)
    signal.signal(signal.SIGTERM, terminationSignalreceived)
