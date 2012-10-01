#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <LUFA/Drivers/USB/USB.h>

#include "ftdi.h"

static void setup(void)
{
	/* Divide 8MHz clock by 1 */
	clock_prescale_set(clock_div_1);

	/* Initialize USB */
	USB_Init();

	/* Initialize USB serial and USB interrupts */
	ftdi_init(FTDI_STDIO | FTDI_BLOCKING);

	/* Ready to go */
	sei();
}

int main(void)
{
	int c;

	setup();

	printf("Hello, world!\n");
	for (;;) {
		c = getchar();
		/* If FTDI_NONBLOCKING was provided to ftdi_init,
		   c will be -1 if no data was available. */
		if (c >= 0)
			printf("You sent %d (%c)\n", c, isprint(c) ? c : '?');
	}
}
