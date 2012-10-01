#include <stdint.h>
#include <avr/wdt.h>
#include <util/delay.h>

/* Early boot stuff.  Save the reset reason and disable watchdog. */
static uint8_t saved_mcusr __attribute__ ((section (".noinit")));

void __reset_early(void)
	__attribute__((naked))
	__attribute__((section(".init3")));
void __reset_early(void)
{
	/* Ensure that the watchdog is disabled.  Otherwise, clearing
	   BSS can take so long that the watchdog will trigger a reset
	   before we even get to main!  Save the reset cause as well,
	   so we can return it later. */
	saved_mcusr = MCUSR;
	MCUSR = 0;
	wdt_disable();
}
