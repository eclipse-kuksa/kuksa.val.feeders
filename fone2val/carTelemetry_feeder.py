#################################################################################
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
#################################################################################

import os
import sys
import signal
import threading
import configparser
from kuksa_client.grpc import VSSClient
from kuksa_client.grpc import Datapoint
from telemetry_f1_2021.listener import TelemetryListener

scriptDir = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(scriptDir, "../../"))


class Kuksa_Client():
    # Constructor
    def __init__(self, config):
        print("Init kuksa client...")
        if "kuksa_val" not in config:
            print("kuksa_val section missing from configuration, exiting")
            sys.exit(-1)

    def shutdown(self):
        self.client.stop()

# Christophers approach on sending Data to Kuksa Server
    def setTelemetryData(self, teleData):
        dataDictionary = {}

        kuksaConfig = config['kuksa_val']
        with VSSClient(kuksaConfig.get('host'), kuksaConfig.getint('port')) as client:
            for x, y in teleData.items():
                dataDictionary.update({
                    str(x): Datapoint(y)
                })
            client.set_current_values(dataDictionary)


class carTelemetry_Client():

    def __init__(self, config, consumer):
        print("Init carTelemetry client...")
        if "listenerIPAddr" not in config:
            print("listenerIPAddr section missing from configuration, exiting")
            sys.exit(-1)
        if "PS5_UDPPort" not in config:
            print("PS5_UDPPort section missing from configuration, exiting")
            sys.exit(-1)
        # extract carTelemetry Data
        print("Connecting to extract CarTelemetry Data")

        self.carTelemetry = {}
        self.running = True

        self.thread = threading.Thread(target=self.loop, args=())
        self.thread.start()

    def loop(self):
        print("Car Telemetry data loop started")

        config_ipAddr = config['listenerIPAddr']
        config_UDPport = config['PS5_UDPPort']

        listener_ip = config_ipAddr['host']
        udp_port = config_UDPport['port']

        print(f"listener_ip:{listener_ip}")
        print(f"udp_port:{udp_port}")

        listener = TelemetryListener(port=int(udp_port), host=listener_ip)

        while self.running:
            try:
                # listen to the data via UDP channel
                packet = listener.get()

                # Update packet ID
                packetID = packet.m_header.m_packet_id
                # player carIndex
                carIndex = packet.m_header.m_player_car_index
                # Check for telemetry data - packet ID 6.
                if (packetID == 6):

                    EngineRPM = packet.m_car_telemetry_data[carIndex].m_engine_rpm
                    Speed = packet.m_car_telemetry_data[carIndex].m_speed

                    self.carTelemetry['Vehicle.Speed'] = Speed
                    self.carTelemetry['Vehicle.RPM'] = EngineRPM

                    # Set the data to the KUKSA_VAL
                    self.consumer.setTelemetryData(self.carTelemetry)

                if (packetID == 7):  # car status data packet
                    fuelInTank = packet.m_car_status_data[carIndex].m_fuel_in_tank
                    fuelCapacity = packet.m_car_status_data[carIndex].m_fuel_capacity
                    fuelInPercent = fuelInTank/fuelCapacity

                    self.carTelemetry['Vehicle.FuelLevel'] = int(fuelInPercent*100)
                    self.consumer.setTelemetryData(self.carTelemetry)

                if (packetID == 10):  # car dmg packet

                    leftWingDamage = packet.m_car_damage_data[carIndex].m_front_left_wing_damage
                    rightWingDamage = packet.m_car_damage_data[carIndex].m_front_right_wing_damage

                    tyreWear_1 = packet.m_car_damage_data[carIndex].m_tyres_wear[0]
                    tyreWear_2 = packet.m_car_damage_data[carIndex].m_tyres_wear[1]
                    tyreWear_3 = packet.m_car_damage_data[carIndex].m_tyres_wear[2]
                    tyreWear_4 = packet.m_car_damage_data[carIndex].m_tyres_wear[3]

                    self.carTelemetry['Vehicle.FrontLeftWingDamage'] = leftWingDamage
                    self.carTelemetry['Vehicle.FrontRightWingDamage'] = rightWingDamage
                    self.carTelemetry['Vehicle.Tire.RearLeftWear'] = tyreWear_1
                    self.carTelemetry['Vehicle.Tire.RearRightWear'] = tyreWear_2
                    self.carTelemetry['Vehicle.Tire.FrontLeftWear'] = tyreWear_3
                    self.carTelemetry['Vehicle.Tire.FrontRightWear'] = tyreWear_4

                    self.consumer.setTelemetryData(self.carTelemetry)
                if (packetID == 2):
                    lastLapTime = packet.m_lap_data[carIndex].m_last_lap_time_in_ms

                    self.carTelemetry['Vehicle.LastLapTime'] = lastLapTime/1000

                    self.consumer.setTelemetryData(self.carTelemetry)
            except Exception:
                continue

    def shutdown(self):
        self.running = False
        self.consumer.shutdown()
        self.carTelemetry.close()
        self.thread.join()


if __name__ == "__main__":
    print("<kuksa.val> Car Telemetry example feeder")
    config_candidates = ['/config/carTelemetry_feeder.ini',
                         '/etc/carTelemetry_feeder.ini',
                         os.path.join(scriptDir, 'config/carTelemetry_feeder.ini')]
    for candidate in config_candidates:
        if os.path.isfile(candidate):
            configfile = candidate
            break
    if configfile is None:
        print("No configuration file found. Exiting")
        sys.exit(-1)
    config = configparser.ConfigParser()
    config.read(configfile)

    client = carTelemetry_Client(config, Kuksa_Client(config))

    def terminationSignalreceived(signalNumber, frame):
        print("Received termination signal. Shutting down")
        client.shutdown()
    signal.signal(signal.SIGINT, terminationSignalreceived)
    signal.signal(signal.SIGQUIT, terminationSignalreceived)
    signal.signal(signal.SIGTERM, terminationSignalreceived)

# end of file #
