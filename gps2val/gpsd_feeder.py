#! /usr/bin/env python

########################################################################
# Copyright (c) 2020 Robert Bosch GmbH
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
import gps

scriptDir= os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(scriptDir, "../../"))
from kuksa_viss_client import KuksaClientThread

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
        print("authorizing...")
        self.client.authorize()
        
    def shutdown(self):
        self.client.stop()

    def setPosition(self, position):
        self.client.setValue('Vehicle.CurrentLocation.Altitude', str(position['alt']))
        self.client.setValue('Vehicle.CurrentLocation.Latitude', str(position["lat"]))
        self.client.setValue('Vehicle.CurrentLocation.Longitude', str(position["lon"]))
        self.client.setValue('Vehicle.CurrentLocation.HorizontalAccuracy', str(position["hdop"]))
        self.client.setValue('Vehicle.Speed', str(position["speed"]))

class GPSD_Client():
    def __init__(self, config, consumer):
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
        self.gpsd = gps.gps(host = self.gpsd_host, port = self.gpsd_port, mode=gps.WATCH_ENABLE)

        self.position = {"lat":"0", "lon":"0", "alt":"0", "hdop":"0", "speed":"0" }
        self.running = True

        self.thread = threading.Thread(target=self.loop, args=())
        self.thread.start()

    def loop(self):
        print("gpsd receive loop started")
        while self.running:
            try:
                if self.gpsd.waiting():
                    report = self.gpsd.next()
                    if report['class'] == 'TPV':

                        self.position['lat']= getattr(report,'lat',0.0)
                        self.position['lon']= getattr(report,'lon',0.0)
                        self.position['alt']= getattr(report,'alt', 'nan')
                        self.position['speed']= getattr(report,'speed',0.0)
                        print(getattr(report,'time', "-"))
                        print(self.position)
                        self.consumer.setPosition(self.position)
                time.sleep(self.interval) 
            except Exception as e:
                print("Got exception: ")
                print(e)
                time.sleep(self.interval) 
                continue

     


    def shutdown(self):
        self.running=False
        self.consumer.shutdown()
        self.gpsd.close()
        self.thread.join()

        
if __name__ == "__main__":
    config_candidates=['/config/gpsd_feeder.ini', '/etc/gpsd_feeder.ini', os.path.join(scriptDir, 'config/gpsd_feeder.ini')]
    for candidate in config_candidates:
        if os.path.isfile(candidate):
            configfile=candidate
            break
    if configfile is None:
        print("No configuration file found. Exiting")
        sys.exit(-1)
    config = configparser.ConfigParser()
    config.read(configfile)
    
    client = GPSD_Client(config, Kuksa_Client(config))

    def terminationSignalreceived(signalNumber, frame):
        print("Received termination signal. Shutting down")
        client.shutdown()
    signal.signal(signal.SIGINT, terminationSignalreceived)
    signal.signal(signal.SIGQUIT, terminationSignalreceived)
    signal.signal(signal.SIGTERM, terminationSignalreceived)

