#! /usr/bin/env python

################################
#Car telemetry feeder
################################

import os, sys, json, signal
import pickle
import re
import threading
import configparser
import time
import queue

from kuksa_viss_client import KuksaClientThread

from telemetry_f1_2021.packets import HEADER_FIELD_TO_PACKET_TYPE

from telemetry_f1_2021.listener import TelemetryListener

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

# VSS model path needs to be checked and updated
    def setTelemetryData(self, carTelemetry):
        self.client.setValue('Vehicle.Powertrain.ElectricMotor.Motor.Rpm', carTelemetry['Engine_RPM'])
        self.client.setValue('Vehicle.Powertrain.CombustionEngine.Engine.Speed', carTelemetry["Car_Speed"])
        
    def getTelemetryData(self, carTelemetry):
        rpm = self.client.getValue('Vehicle.Powertrain.ElectricMotor.Motor.Rpm', carTelemetry['Engine_RPM'])
        speed = self.client.getValue('Vehicle.Powertrain.CombustionEngine.Engine.Speed', carTelemetry["Car_Speed"])
        
        return rpm,speed
        
class carTelemetry_Client():

    def __init__(self, config, consumer):
        print("Init carTelemetry client...")
        
        if "carTelemetry" not in config:
            print("carTelemetry section missing from configuration, exiting")
            sys.exit(-1)
        
        self.consumer = consumer
        provider_config=config['carTelemetry']
        self.interval = provider_config.getint('interval', 1)
        
        #extract carTelemetry Data
        print("Connecting to extract CarTelemetry Data")
        
        self.carTelemetry = {"Engine_RPM":"0", "Car_Speed":"0"}
        self.running = True

        self.thread = threading.Thread(target=self.loop, args=())
        self.thread.start()

    def loop(self):
        print("Car Telemetry data loop started")
        
        #listenData = True
        
        listener = TelemetryListener(port=20777, host='192.168.178.169')
        directoryPath =  "/home/harishnr/kuksaVal/kuksa.val/kuksa_feeders/carTelemetry2val/config/CTP/"
        
        while self.running:
            try:
                print("Updating.. Car Telemetry")    
                
                #listen to the data via UDP channel
                packet = listener.get()
                
                telemetry = {}
                telemetry['telemetryData'] = []
                index = 0 

                #Update packet ID
                packetID = packet.m_header.m_packet_id
                
                #Check for telemetry data - packet ID 6.
                if (packetID == 6):
                    carIndex = packet.m_header.m_player_car_index
                    #frameID = packet.m_header.m_frame_identifier
                        
                    EngineRPM = packet.m_car_telemetry_data[carIndex].m_engine_rpm
                    Speed = packet.m_car_telemetry_data[carIndex].m_speed
                        
                    index = index+1
                        
                    self.carTelemetry['Engine_RPM']= EngineRPM
                    self.carTelemetry['Car_Speed']= Speed
                    print ('setData') 
                    print(index)
                    print(self.carTelemetry)
                    
                    #Set the data to the KUKSA_VAL
                    self.consumer.setTelemetryData(self.carTelemetry)
                    
                    #Creating JSON file for test purpose   
                    telemetry['telemetryData'].append({
                                'CarSpeed': Speed ,
                                'EngineRPM': EngineRPM
                    })
           
      		 # updating telemetry data to a JSON file
                with open('telemetryData.json', 'w') as outfile:
                    json.dump(telemetry, outfile, indent=4)
            
    
            except Exception as e:
                print("Get exceptions: ")
                print(e)
                time.sleep(self.interval) 
                continue


    def shutdown(self):
        self.running=False
        self.consumer.shutdown()
        self.carTelemetry.close()
        self.thread.join()
           
if __name__ == "__main__":
    print("<kuksa.val> Car Telemetry example feeder")
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
    
    client = carTelemetry_Client(config, Kuksa_Client(config))

    def terminationSignalreceived(signalNumber, frame):
        print("Received termination signal. Shutting down")
        client.shutdown()
    signal.signal(signal.SIGINT, terminationSignalreceived)
    signal.signal(signal.SIGQUIT, terminationSignalreceived)
    signal.signal(signal.SIGTERM, terminationSignalreceived)

# end of file #
