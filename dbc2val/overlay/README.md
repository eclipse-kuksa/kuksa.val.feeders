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
  dbc: VCFRONT_passengerPresent
  dbc_mapping: [{1:true},{0:false}]
  dbc_changetype: ON_CHANGE
  dbc_interval_ms: 1000
```

Which could (theoretically, see below,) be called like:

```
./vss-tools/vspec2json.py --no-uuid -e dbc,dbc_mapping,dbc_changetype,dbc_interval_ms ./spec/VehicleSignalSpecification.vspec -o overlay.vspec --json-pretty vss.json
```

This gives this resulting JSON:

```
                      "IsOccupied": {
                        "datatype": "boolean",
                        "dbc": "VCFRONT_passengerPresent",
                        "dbc_changetype": "ON_CHANGE",
                        "dbc_interval_ms": 1000,
                        "dbc_mapping": [
                          {
                            "$file_name$": "overlay.vspec",
                            "$line$": 5,
                            "$name$": "1:true"
                          },
                          {
                            "$file_name$": "overlay.vspec",
                            "$line$": 5,
                            "$name$": "0:false"
                          }
                        ],
                        "description": "Does the seat have a passenger in it.",
                        "type": "actuator",
                        "uuid": "4d0cdff266e45dd2a8a878b572d34b7e"
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

### Alternative representation
We could also have a tree-like representation, similar to what is used today


```
Vehicle.Cabin.Seat.IsOccupied:
  type: actuator
  datatype: boolean
  dbc: VCFRONT_passengerPresent
  dbc_mapping:
    value:
      "1": true
      "0": false
  dbc_changetype: ON_CHANGE
  dbc_interval_ms: 1000
```

Which in generated json would result in something like:

```
  "dbc": "VCFRONT_passengerPresent",
  "dbc_interval_ms": 1000,
  "dbc_mapping": {
     "$file_name$": "overlay.vspec",
     "$line$": 5,
     "value": {
       "$file_name$": "overlay.vspec",
       "$line$": 6,
       "0": false,
       "1": true
   }
```

An important limitation in vss-tools is that keys cannot be integers, i.e. it is not possible to specify `1: true`,
you must specify `"1": true`. String values must be quoted, as they otherwise due to Yaml interpretation may be interpreted wrongly.
An example is `off` which is a synonym for `false`in Yaml, so `dbc: off` may be generated as `dbc: false`. To prevent this quotes shall preferably be used for string literals, like `dbc: "off"`

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
  dbc: SteeringAngle129
  dbc_math: "floor(x+0.5)"
  dbc_changetype: ON_CHANGE
  dbc_interval_ms: 100
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
  dbc: SteeringAngle129
  dbc_mapping: [{DI_GEAR_D:1},{DI_GEAR_P:0},{DI_GEAR_INVALID:0},{DI_GEAR_R:-1}]
  dbc_changetype: ON_CHANGE
  dbc_interval_ms: 100
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
  dbc: VCFRONT_brakeFluidLevel
  dbc_mapping: [{LOW:true},{NORMAL:false}}]
  dbc_changetype: ON_CHANGE
  dbc_interval_ms: 1000
  
  
Vehicle.Chassis.Axle.Row1.Wheel.Right.Brake.IsFluidLevelLow:
  type: sensor
  datatype: boolean
  dbc: VCFRONT_brakeFluidLevel
  dbc_mapping: [{LOW:true},{NORMAL:false}}]
  dbc_changetype: ON_CHANGE
  dbc_interval_ms: 1000
  
  ... (2 signals omitted)
```

Alternatively, one could here make an overlay before expansion (only method currently supported) which will exist for all after expansion.


```

Vehicle.Chassis.Axle.Wheel.Brake.IsFluidLevelLow:
  type: sensor
  datatype: boolean
  dbc: VCFRONT_brakeFluidLevel
  dbc_mapping: [{LOW:true},{NORMAL:false}}]
  dbc_changetype: ON_CHANGE
  dbc_interval_ms: 1000
  

```
