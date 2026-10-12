# Camera, OLED & PMU

## OV2640 camera

At startup the firmware initializes the camera through the reusable driver.
The recorded run detected an OV2640 and initialized it successfully. Current
validation captures JPEG at 320 × 240.

## SSD1306 OLED

The optional OLED is registered on the existing I²C bus at address `0x3C`.
Firmware logs that the OLED responded and writes a test pattern.

**Communication detection is not visual verification.** An I²C response and a
successful driver write do not confirm that a person saw the expected pixels
on the physical display. The recorded validation evidence establishes
communication, not independent optical inspection.

## AXP2101 PMU

The firmware reads the PMU chip ID register. The reported AXP2101 ID is
`0x4A`. Only when that ID matches does it configure the existing camera rails:

| Rail | Configured voltage |
| --- | ---: |
| ALDO1 | 1500 mV |
| ALDO2 | 3000 mV |
| ALDO4 | 3000 mV |

Other PMU outputs are not changed by this camera rail configuration. The code
uses the project’s factory settings; do not substitute another board preset
without verifying the hardware requirements.
