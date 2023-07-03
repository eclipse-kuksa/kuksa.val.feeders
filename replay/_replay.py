########################################################################
# Copyright (c) 2021 Robert Bosch GmbH
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

import sys
import os
import time
import datetime
import traceback
import configparser
import csv
from kuksa_client import KuksaClientThread

scriptDir = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(scriptDir, "..", ".."))

# Expected columns in log file from KUKSA.val Server
rowIDs = [
    "timestamp",
    "ID",
    "action",
    "attribute",
    "path",
    "value"
]

try:
    config = configparser.ConfigParser()
    config.read('config.ini')
except Exception:
    print("Unable to read config file")
    os._exit(0)

args = config['replay']               # get replay data
vsscfg = config['vss']              # get Client data from config file
csv_path = args.get('path')

try:
    commThread = KuksaClientThread(vsscfg)       # make new thread
    commThread.start()
    commThread.authorize()
    connected = commThread.checkConnection()
    if not connected:
        print("Could not connect successfully")
        sys.exit(-1)
    print("Connected successfully")
except Exception:
    print("Exception when trying to connect")
    sys.exit(-1)

try:

    if not args.get('mode') in ['Set', 'SetGet']:
        raise AttributeError

    with open(csv_path, "r") as recordFile:
        print("Replaying data from " + csv_path)
        fileData = csv.DictReader(recordFile, rowIDs, delimiter=';')

        timestamp_pre = 0
        for row in fileData:
            timestamp_curr = row["timestamp"]

            if timestamp_pre != 0:
                curr = datetime.datetime.strptime(timestamp_curr, '%Y-%b-%d %H:%M:%S.%f')
                pre = datetime.datetime.strptime(timestamp_pre, '%Y-%b-%d %H:%M:%S.%f')
                delta = (curr-pre).total_seconds()          # get time delta between the timestamps
            else:
                delta = 0

            timestamp_pre = timestamp_curr

            time.sleep(delta)
            if row['action'] == 'set':
                commThread.setValue(row['path'], row['value'])
            elif args.get('mode') == 'SetGet':
                commThread.getValue(row['path'])

        print("Replay successful")

except AttributeError:
    print("Wrong attributes used. Please check config.ini")

except Exception:
    traceback.print_exc()

os._exit(1)
