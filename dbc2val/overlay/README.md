# Using overlays as alternative mapping

## Example 1 - Boolean

```
VCFRONT_passengerPresent:
  vss: # taken from https://github.com/COVESA/vehicle_signal_specification/blob/master/spec/Cabin/SingleSeat.vspec
    datatype: boolean 
    type: sensor
    unit: none
    description: Does the seat have a passenger in it.
  databroker:
    datatype: 1 # BOOL number taken from types_pb2.py
    changetype: 2 # ON_CHANGE number taken from types_pb2.py
  minupdatedelay: 1000
  targets:
    Vehicle.Cabin.Seat.Row1.Pos2.IsOccupied:
      transform:
        fullmapping:
          1: "true"
          0: "false"
```

A functionally equal mapping could possibly be like this:

```
Vehicle.Cabin.Seat.Row1.Pos2.IsOccupied:
  type: actuator
  datatype: boolean
  dbc:
    signal: VCFRONT_passengerPresent
    transform:
      mapping:
        - from: 0
          to: false
        - from: 1
          to: true
    changetype: ON_CHANGE
    interval_ms: 1000
```

Which could (theoretically, see below,) be called like:

```
./vss-tools/vspec2json.py --no-uuid -e dbc,dbc_mapping,dbc_changetype,dbc_interval_ms ./spec/VehicleSignalSpecification.vspec -o overlay.vspec --json-pretty vss.json
```

This gives this resulting JSON:

```
"IsOccupied": {
                        "datatype": "boolean",
                        "dbc": {
                          "changetype": "ON_CHANGE",
                          "interval_ms": 1000,
                          "signal": "VCFRONT_passengerPresent",
                          "transform": {
                            "mapping": [
                              {
                                "from": 0,
                                "to": false
                              },
                              {
                                "from": 1,
                                "to": true
                              }
                            ]
                          }
                        },

```

Notes:
* As of today the VSS overlay mechanism cannot be used on expanded branches like `Vehicle.Cabin.Seat.Row1.Pos2.IsOccupied`.
  Creating a syntax that that works on "unexpanded" nodes like `Vehicle.Cabin.Seat.IsOccupied` would mean more work,'
  both for defining the syntax and also for using it within dbc2val feeder. VSS-Tools [issue](https://github.com/COVESA/vss-tools/issues/201) created for discussion.
* The term `minupdatedelay` currently used might be misleading.
  Rather than specifying a delay, or a minimum interval, it specifies a wanted interval (for CONTINUOUS).
  I.e. if you specify 1000, you want to get an update about every 1000 ms, you do not want it every 2000 ms.
  For `ON_CHANGE` ita minimum interval.


## Example 2 -int with math formula

```
#Steering angle
# Note VSS is angle of the steering wheel, DBC might be angle or steering?
# Converting to int
SteeringAngle129:
  vss: # taken from https://github.com/COVESA/vehicle_signal_specification/blob/master/spec/Chassis/Chassis.vspec
    datatype: int16 
    type: sensor
    unit: degrees
    description: Steering wheel angle. Positive = degrees to the left. Negative = degrees to the right.
  databroker:
    datatype: 3 # INT16 number taken from types_pb2.py
    changetype: 2 # ON_CHANGE number taken from types_pb2.py
  minupdatedelay: 100
  targets:
    Vehicle.Chassis.SteeringWheel.Angle:
      transform:
        math: "floor(x+0.5)"
        
```

A functionally equal mapping could possibly be like this:

```
Vehicle.Chassis.SteeringWheel.Angle:
  type: sensor
  datatype: int16
  dbc:
    signal: SteeringAngle129
    transform:
      mapping:
        - math: "floor(x+0.5)
    changetype: ON_CHANGE
    interval_ms: 100
```

## Example 3 - DBC enum to VSS int

```
#"DI_GEAR_D" 0 "DI_GEAR_INVALID" 3 "DI_GEAR_N" 1 "DI_GEAR_P" 2 "DI_GEAR_R" 7 "DI_GEAR_SNA" ;
DI_gear:
  vss: # taken from https://github.com/COVESA/vehicle_signal_specification/blob/master/spec/Powertrain/Transmission.vspec
    datatype: int8
    type: sensor
    unit: none
    description: The current gear. 0=Neutral, 1/2/..=Forward, -1/-2/..=Reverse
  databroker:
    datatype: 2 # INT8 number taken from types_pb2.py
    changetype: 2 # ON_CHANGE number taken from types_pb2.py
  minupdatedelay: 100
  targets:
    Vehicle.Powertrain.Transmission.CurrentGear:
      transform:
        fullmapping:
          DI_GEAR_D: 1
          DI_GEAR_INVALID: 0
          DI_GEAR_P: 0
          DI_GEAR_R: -1
```

``

A functionally equal mapping could possibly be like this (using ENUM rather than value):

```
Vehicle.Powertrain.Transmission.CurrentGear:
  type: sensor
  datatype: int16
  dbc:
    signal: DI_gear
    transform:
      mapping:
        - from: DI_GEAR_D
          to: 1
        - from: DI_GEAR_P
          to: 0
        - from: DI_GEAR_INVALID
          to: 0
        - from: DI_GEAR_R
          to: -1
    changetype: ON_CHANGE
    interval_ms: 100
```

## Example 4 - Mapping one DBC signal to multiples VSS signals
```
VCFRONT_brakeFluidLevel:
  vss: # taken from https://github.com/COVESA/vehicle_signal_specification/blob/master/spec/Chassis/Wheel.vspec
    datatype: boolean
    type: sensor
    unit: none
    description: Brake fluid level status. True = Brake fluid level low. False = Brake fluid level OK.
  databroker:
    datatype: 1 # BOOL number taken from types_pb2.py
    changetype: 2 # ON_CHANGE number taken from types_pb2.py
  minupdatedelay: 1000
  targets:
    Vehicle.Chassis.Axle.Row1.Wheel.Left.Brake.IsFluidLevelLow:
      transform:
        fullmapping:
          LOW: "true"
          NORMAL: "false"
    Vehicle.Chassis.Axle.Row1.Wheel.Right.Brake.IsFluidLevelLow:
      transform:
        fullmapping:
          LOW: "true"
          NORMAL: "false"
    Vehicle.Chassis.Axle.Row2.Wheel.Left.Brake.IsFluidLevelLow:
      transform:
        fullmapping:
          LOW: "true"
          NORMAL: "false"
    Vehicle.Chassis.Axle.Row2.Wheel.Right.Brake.IsFluidLevelLow:
      transform:
        fullmapping:
          LOW: "true"
          NORMAL: "false"
```

A functionally equal mapping needs to repeated for every VSS signal.

```

Vehicle.Chassis.Axle.Row1.Wheel.Left.Brake.IsFluidLevelLow:
  type: sensor
  datatype: boolean
  dbc:
    signal: VCFRONT_brakeFluidLevel
    transform:
      mapping:
        - from: LOW
          to: true
        - from: NORMAL
          to: false
    changetype: ON_CHANGE
    interval_ms: 1000
  
  
Vehicle.Chassis.Axle.Row1.Wheel.Right.Brake.IsFluidLevelLow:
  type: sensor
  datatype: boolean
  dbc:
    signal: VCFRONT_brakeFluidLevel
    transform:
      mapping:
        - from: LOW
          to: true
        - from: NORMAL
          to: false
    changetype: ON_CHANGE
    interval_ms: 1000
  
  ... (2 signals omitted)
```

Alternatively, one could here make an overlay before expansion (only method currently supported) which will exist for all after expansion.


```

Vehicle.Chassis.Axle.Wheel.Brake.IsFluidLevelLow:
  type: sensor
  datatype: boolean
  dbc:
    signal: VCFRONT_brakeFluidLevel
    transform:
      mapping:
        - from: LOW
          to: true
        - from: NORMAL
          to: false
    changetype: ON_CHANGE
    interval_ms: 1000
  

```
