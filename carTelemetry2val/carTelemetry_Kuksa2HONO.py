#! /usr/bin/env python
#  Copyright (c) 2020 Contributors to the Eclipse Foundation
#
#  See the NOTICE file(s) distributed with this work for additional
#  information regarding copyright ownership.
#
#  This program and the accompanying materials are made available under the
#  terms of the Eclipse Public License 2.0 which is available at
#  http://www.eclipse.org/legal/epl-2.0
#
#  SPDX-License-Identifier: EPL-2.0

from __future__ import print_function, unicode_literals

import os, sys
import signal
import json
import threading
import time
import configparser
import kuksa_viss_client as kuksaVal
from kuksa_viss_client import KuksaClientThread

import requests
import paho.mqtt.publish as publish
from proton.handlers import MessagingHandler
from proton.reactor import Container
from requests.auth import HTTPBasicAuth

registryIp = "hono.eclipseprojects.io"
httpAdapterIp = "hono.eclipseprojects.io"
mqttAdapterIp = "hono.eclipseprojects.io"
amqpNetworkIp = "hono.eclipseprojects.io"

# Register Tenant
tenant = requests.post(f'http://{registryIp}:28080/v1/tenants').json()
tenantId = tenant["id"]

print(f'Registered tenant {tenantId}')

# Add Device to Tenant
device = requests.post(f'http://{registryIp}:28080/v1/devices/{tenantId}').json()
deviceId = device["id"]

print(f'Registered device {deviceId}')

# Set Device Password
devicePassword = "my-secret-password"

code = requests.put(f'http://{registryIp}:28080/v1/credentials/{tenantId}/{deviceId}',
                    headers={'content-type': 'application/json'},
                    data=json.dumps(
                        [{"type": "hashed-password", "auth-id": deviceId, "secrets": [{"pwd-plain": devicePassword}]}]))

if code.status_code == 204:
    print("Password is set!")
else:
    print("Unable to set Password")

# Now we can start the client application
print("We could use the Hono Client now...")
print()
cmd = f'java -jar hono-cli-*-exec.jar --hono.client.host={amqpNetworkIp} ' \
    f'--hono.client.port=15672 --hono.client.username=consumer@HONO ' \
    f'--hono.client.password=verysecret --spring.profiles.active=receiver ' \
    f'--tenant.id={tenantId}'
print(cmd)
print()


# input("Press Enter to continue...")

class AmqpHandler(MessagingHandler):
    """
    Handler for "northbound side" where Messages are received
    via AMQP.
    """
    def __init__(self, server, address):
        super(AmqpHandler, self).__init__()
        self.server = server
        self.address = address

    def on_start(self, event):
        conn = event.container.connect(self.server, user="consumer@HONO", password="verysecret")
        event.container.create_receiver(conn, self.address)

    def on_connection_error(self, event):
        print("Connection Error")

    def on_link_error(self, event):
        print("Link Error")

    def on_message(self, event):
        print("Got a message:")
        print(event.message.body)


# Prepare the container
uri = f'amqp://{amqpNetworkIp}:15672'
address = f'telemetry/{tenantId}'
print("Using source: " + uri)
print("Using address: " + address)
container = Container(AmqpHandler(uri, address))

# run container in separate thread
print("Starting (northbound) AMQP Connection...")
thread = threading.Thread(target=lambda: container.run(), daemon=True)
thread.start()

# Give it some time to link
time.sleep(2)

scriptDir= os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(scriptDir, "../../"))

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
        
# Get VSS data
        
    def getTelemetryData(self):
        
        telemetryData1 = self.client.getValue('Vehicle.Powertrain.ElectricMotor.Motor.Rpm')
        r_engineRPM = json.loads(telemetryData1)
        telemetryData2 = self.client.getValue('Vehicle.Powertrain.CombustionEngine.Engine.Speed')
        r_carSpeed = json.loads(telemetryData2)
        engineRPM = str(r_engineRPM["data"]["dp"]["value"])
        carSpeed = str(r_carSpeed["data"]["dp"]["value"])
        
        return engineRPM,carSpeed

class mqtt_HONOClient():
                                                                      
    def __init__(self, config, consumer):
        print("Init mqtt HONO client...")

        if "carTelemetryHONO" not in config:
            print("carTelemetryHONO section missing from configuration, exiting")
            sys.exit(-1)
        
        self.consumer = consumer
        provider_config=config['carTelemetryHONO']
        self.interval = provider_config.getint('interval', 1)
        
        self.carTelemetry = {"Engine_RPM":"0", "Car_Speed":"0"}
        self.running = True
        
        self.thread = threading.Thread(target=self.loop, args=())
        self.thread.start()
     
    def loop(self):
        
        client_Obj = Kuksa_Client(config)
        
        engineRPM_Old = 0
        carSpeed_Old = 0
        
        while self.running:
            
            engineRPM,carSpeed = client_Obj.getTelemetryData()
            
            if carSpeed_Old != carSpeed or engineRPM_Old != engineRPM:
                # Send Message via MQTT
                print("Send Telemetry Message via MQTT")
                publish.single("telemetry", payload=json.dumps({"CarSpeed": carSpeed,"EngineRPM": engineRPM, "transport": "mqtt"}),
                    hostname=mqttAdapterIp,
                    auth={"username": f'{deviceId}@{tenantId}', "password": devicePassword})
                
                engineRPM_Old = engineRPM
                carSpeed_Old = carSpeed
            
                # Wait a bit for the MQTT Message to arrive
                time.sleep(2)  
    
if __name__ == "__main__":
    print("<kuksa.val> Car Telemetry HONO feeder")
    config_candidates=['/config/carTelemetry_feeder.ini', '/etc/carTelemetry_feeder.ini', os.path.join(scriptDir, 'config/carTelemetry_feeder.ini')]
    for candidate in config_candidates:
        if os.path.isfile(candidate):
            configfile=candidate
            break
    if configfile is None:
        print("No configuration file found. Exiting")
        sys.exit(-1)
    config = configparser.ConfigParser()
    config.read(configfile)
    
    client = mqtt_HONOClient(config, Kuksa_Client(config))

    def terminationSignalreceived(signalNumber, frame):
        
        print("\n Received termination signal \n.")
        
        print("Stopping (northbound) AMQP Connection... \n")
        
        #Stop container
        container.stop() 
        thread.join(timeout=5)
        
        print("Shutting down Client \n")
        client.shutdown()
    
    signal.signal(signal.SIGINT, terminationSignalreceived)
    signal.signal(signal.SIGQUIT, terminationSignalreceived)
    signal.signal(signal.SIGTERM, terminationSignalreceived)

# end of file #
