/*
* ### SuperCaps UPS Controller ###
*
* main.cpp
*
* Created: 10/26/2019 4:10:48 PM
* Author : IT Logic
* 
* License: https://creativecommons.org/licenses/by-nc-sa/4.0/
*/

#include <avr/io.h>
#include <stdint.h>

//#define F_CPU 1000000UL  // Defined in platformio.ini
#include <util/delay.h>

/**** Static values ****/
#define RUN_PIN PB2
#define PWROFF_PIN PB0
#define SHTDWN_PIN PB1
#define PWFAIL_PIN PB4
#define DAC_PIN ADC3D

#define MIN_VCAP 3600U // Minimum SuperCaps voltage in mV to start shutdown
#define ENG_COEFF 29U  // Multiply the raw value of the DAC to have the voltage in mV using a voltage divider ratio of 3, Vref of 2.56V and 8bit resolution
                       // The coefficient has been lowered to 29 to match the prototype measurement. Since is used a Vref without ext cap the precision is not optimal

/**** Enumerators Declaration ****/
typedef enum
{
	INIT,
	STARTUP,
	WAITING,
	WAIT_HALT
} SCAP_PI_STATE;

int main(void)
{
	// Initialize global Variables
	SCAP_PI_STATE state = INIT; // Instantiate the state enumerator

	// Initialize the I/O Port
	DDRB |= (1 << RUN_PIN) | (1 << SHTDWN_PIN); // RUN and SHTDWN as Output pin
	PORTB |= (1 << PWFAIL_PIN); // 				// PullUp on PWFAIL_PIN
	DIDR0 |= (1 << DAC_PIN);					// Disable the digital input is used as analog input

	// Initialize the DAC
	ADMUX =
		(1 << ADLAR) | // Left shift result
		//(0 << ADLAR) | // Right shift result for 10 bit DAC
		(1 << REFS2) | // Internal 2.56V Voltage Reference without external bypass capacitor
		(1 << REFS1) |
		(0 << REFS0) |
		(0 << MUX3) | // use ADC3 for input (PB3)
		(0 << MUX2) |
		(1 << MUX1) |
		(1 << MUX0);

	ADCSRA =
		(1 << ADEN) |  // Enable ADC
		(1 << ADPS2) | // set ADC clock pre scaler to 64
		(1 << ADPS1) |
		(0 << ADPS0);

	// Main loop
	while (1)
	{
		switch (state)
		{
		case INIT: // Initialize the outputs
		{
			PORTB &= ~(1 << RUN_PIN);	// Set the Pi in Reset mode
			PORTB &= ~(1 << SHTDWN_PIN); // Clear shutdown signal
			state = STARTUP;
			break;
		}

		case STARTUP: // Startup Mode - Wait till the SuperCaps are fully charged
		{
			if (bit_is_clear(PINB, PWFAIL_PIN))
			{							 // The SuperCap has been charged
				PORTB |= (1 << RUN_PIN); // Allow Raspberry to boot, the SuperCaps are charged
				state = WAITING;
			}
			break;
		}

		case WAITING:
		{
			_delay_ms(1);
			ADCSRA |= (1 << ADSC);				   // start ADC measurement
			loop_until_bit_is_clear(ADCSRA, ADSC); // wait till conversion complete

			// if (((ADCL | (ADCH << 8)) * ENG_COEFF) < MIN_VCAP) // Use 10 Bits ADC *** Note ADCL is fetched before ADCH ***
			if ((ADCH * ENG_COEFF) < MIN_VCAP) // Discard the last two LSB bits in ADCL
			{								// Check if the SuprCaps voltage is still good
				PORTB |= (1 << SHTDWN_PIN); // Signal the raspberry to shutdown
				state = WAIT_HALT;
			}
			break;
		}

		case WAIT_HALT:
		{
			if (bit_is_set(PINB, PWROFF_PIN)) // If the Raspberry Pi has signaled is in Halt
			{
				PORTB &= ~(1 << SHTDWN_PIN);
				PORTB &= ~(1 << RUN_PIN); // Set the Pi in Reset mode
				_delay_ms(10);  // Delay 10ms to have a nice reset in case the power supply has returned and the cap is fully charged
				state = STARTUP;		  // Return to the top and wait power comes back otherwise die with the raspberry
			}
			break;
		}

		default:
			break;
		}
	}
}
