# ATtiny Firmware for RasPi SuperCap UPS & Power Supply

## Follow the project on [Hackaday](https://hackaday.io/project/168748-raspberry-supercapacitor-ups-power-supply)

Program the ATTyny25 **ONLY** when the board is disconnected from the Raspberry.
Most programmers have a +5V power supply, which will damage the GPIO of the Raspberry.

The firmware is a simple state machine which controls the three Raspebbry signal based on the two input from the LTC4041: one is the CAPGD which tells the ATTiny25 the supercapacitors have been used, and the other is an analog voltage which represents the charge voltage of the supercapacitors. At power-on, the interface will keep the Raspberry in reset state till the supercapacitors are fully charged.

The tension at which the supercapacitor trips can be easily changed in software:

`#define MIN_VCAP 3600U    // Minimum SuperCaps voltage in mV to start shutdown`

The ATTiny25 can be programmed with the ICSP connector.
