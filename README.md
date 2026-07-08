# mpy_ugps
Micropython module for GPS receiver module (USER_C_MODULES)


# Constructors
### class ugps.nmea()

```python
from ugps import nmea
parser = nmea()
```

## Methods
### nmea.parse(msg="")
Parses an NMEA message and returns a dictionary of GNSS values.

Parameters:
* msg (str): NMEA message string (e.g., "$GLRMC...*..")

Returns:
* dict: Dictionary containing parsed GNSS values (position, time, speed, etc.)

Example:



* msg - Nmea message

```python

data = parser.parse(msg="$GLRMC...*..")
```
Parse NMEA message
returns dict of gnss values

### nmea.reset()


### nmea.calc_bearing(coord1=[], coord2=[])



* coord1 [lon1,lon2]
* coord2 [lon2,lat2]


### nmea.calc_distance(coord1, coord2)

* coord1 [lon1,lon2]
* coord2 [lon2,lat2]

# class ugps.ublox