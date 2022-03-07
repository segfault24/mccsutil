# mccsutil
A small, Windows-based utility for controlling display monitors via DDC/CI MCCS.

## Examples

- Change monitors 1 and 2 to HDMI 2: `mccsutil input hdmi2 1,2`
- Change all monitors to DisplayPort 1: `mccsutil input dp1`
- Put monitors 3 and 4 on standby: `mccsutil power standby 3,4`
- Turn off all monitors, as if by button: `mccsutil power offb`

## License
This project is released under the MIT License.
