#! /usr/bin/env python

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
import os, sys, json, signal
import csv
import time
import queue
from gpsdclient import GPSDClient
import argparse

scriptDir= os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(scriptDir, "../../"))
from kuksa_client import KuksaClientThread

class Kuksa_Client():

    # Constructor
    def __init__(self, config):
        print("Init kuksa client...")
        if "kuksa_val" not in config:
            print("kuksa_val section missing from configuration, exiting")
            sys.exit(-1)
        provider_config=config['kuksa_val']
        self.client = KuksaClientThread(provider_config)
        self.client.start()
        if str(provider_config.get('protocol')) == 'ws':  # FIXME: and provider_config.get('insecure') != 'True'
            print("authorizing...")
            self.client.authorize(str(provider_config.get('token')))

    def shutdown(self):
        self.client.stop()

    def setData(self, data):
        print(f"Update {data}")
        for k,v in data.items():
            if v is not None:
                self.client.setValue(k,str(v))

class GPSDClientThread(threading.Thread):
    def __init__(self, config, consumer):
        super(GPSDClientThread, self).__init__()
        print("Init gpsd client...")
        if "gpsd" not in config:
            print("gpsd section missing from configuration, exiting")
            sys.exit(-1)

        self.consumer = consumer
        provider_config=config['gpsd']
        self.gpsd_host=provider_config.get('host','127.0.0.1')
        self.gpsd_port=provider_config.get('port','2947')
        self.interval = provider_config.getint('interval', 1)

        print("Trying to connect gpsd at "+str(self.gpsd_host)+" port "+str(self.gpsd_port))
        self.client = GPSDClient(host=self.gpsd_host, port=self.gpsd_port)

        self.collecteddata = {  }
        self.running = True

    def run(self):
        print("gpsd receive loop started")
        for result in self.client.dict_stream(filter=["TPV"]):
            if self.running:
                print("")
                self.collecteddata['Vehicle.CurrentLocation.Latitude']= result.get('lat')
                self.collecteddata['Vehicle.CurrentLocation.Longitude']= result.get('lon')
                self.collecteddata['Vehicle.CurrentLocation.Altitude']= result.get('alt')
                self.collecteddata['Vehicle.Speed']= result.get('speed')
                self.collecteddata['Vehicle.CurrentLocation.Timestamp']= result.get('time')
                self.collecteddata['Vehicle.CurrentLocation.Heading']= result.get('track')
                self.collecteddata['Vehicle.CurrentLocation.HorizontalAccuracy']= result.get('eph')
                self.collecteddata['Vehicle.CurrentLocation.VerticalAccuracy']= result.get('epv')

                self.consumer.setData(self.collecteddata)
                time.sleep(self.interval)
            else:
                print("Exiting")
                break

    def shutdown(self):
        self.running = False
        self.consumer.shutdown()
        print("KUKSA client shutdown")
        self.client.close()
        print("GPSD client shutdown")
        self.join(1)
        if not self.is_alive():
            print("Shutdown completed")
        else:
            print("Shutdown join timed out!")


if __name__ == "__main__":
    manual_config = argparse.ArgumentParser()
    manual_config.add_argument("--host", help="Specify the host where too look for KUKSA.val server/databroker; default: 127.0.0.1", nargs='?' , default="127.0.0.1")
    manual_config.add_argument("--port", help="Specify the port where too look for KUKSA.val server/databroker; default: 8090", nargs='?' , default="8090")
    manual_config.add_argument("--protocol", help="If you want to connect to KUKSA.val server specify ws. If you want to connect to KUKSA.val databroker specify grpc; default: ws", nargs='?' , default="ws")
    manual_config.add_argument("--insecure", help="For KUKSA.val server specify False, for KUKSA.val databroker there is currently no security so specify True; default: False", nargs='?' , default="False")
    manual_config.add_argument("--certificate", help="Specify the path to your Client.pem file; default: Client.pem", nargs='?' , default="Client.pem")
    manual_config.add_argument("--cacertificate", help="Specify the path to your CA.pem; default: CA.pem", nargs='?' , default="CA.pem")
    manual_config.add_argument("--token", help="Specify the path to your JWT token; default: all-read-write.json", nargs='?' , default="all-read-write.json")
    manual_config.add_argument("--file", help="Specify the path to your config file; default: config/gpsd_feeder.ini", nargs='?' , default="config/gpsd_feeder.ini")
    manual_config.add_argument("--gpsd_host", help="Specify the host for gpsd to start on; default: 127.0.0.1", nargs='?' , default="127.0.0.1")
    manual_config.add_argument("--gpsd_port", help="Specify the port for gpsd to start on; default: 2948", nargs='?' , default="2948")
    manual_config.add_argument("--interval", help="Specify the interval time for feeding gps data; default: 1", nargs='?' , default="1")
    args = manual_config.parse_args()
    print(args)
    if os.path.isfile(args.file):
        configfile = args.file
        print("# Using config from: {}".format(configfile))
    else:
        config_object = configparser.ConfigParser()
        print("No configuration file found. Using default values.")
        config_object["kuksa_val"] = {
            "host": args.host,
            "port": args.port,
            "protocol": args.protocol,
            "insecure": args.insecure,
            "certificate": args.certificate,
            "cacertificate": args.cacertificate,
            "token": args.token,
            "file": args.file,
        }
        config_object["gpsd"] = {
            "interval": args.interval,
            "host": args.gpsd_host,
            "port": args.gpsd_port,
        }
        print("\n#<config.ini>:")
        config_object.write(sys.stdout)
        with open('config.ini', 'w') as conf:
            config_object.write(conf)
        configfile = "config.ini"

    config = configparser.ConfigParser()
    config.read(configfile)

    gpsd_client = GPSDClientThread(config, Kuksa_Client(config))
    gpsd_client.start()


    def terminationSignalreceived(signalNumber, frame):
        print("Received termination signal. Shutting down")
        gpsd_client.shutdown()
        os._exit(1)

    signal.signal(signal.SIGINT, terminationSignalreceived)
    signal.signal(signal.SIGTERM, terminationSignalreceived)

