#ifndef FTDI_H
#define FTDI_H

#include <LUFA/Drivers/USB/USB.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* FTDI device-to-host data IN endpoint */
#define FTDI_TX_EPADDR     (ENDPOINT_DIR_IN | 1)
#define FTDI_TX_QUEUE_LEN  256

/* Endpoint address of FTDI host-to-device data OUT endpoint */
#define FTDI_RX_EPADDR     (ENDPOINT_DIR_OUT | 2)
#define FTDI_RX_QUEUE_LEN  256 /* Must be at least FTDI_TXRX_EPSIZE + 1 */

/* Endpoint size */
#define FTDI_TXRX_EPSIZE   16 /* e.g. 16 or 64 */

/* FTDI stuff */
#define SIO_RESET          0 /* Reset the port */
#define SIO_MODEM_CTRL     1 /* Set the modem control register */
#define SIO_SET_FLOW_CTRL  2 /* Set flow control register */
#define SIO_SET_BAUD_RATE  3 /* Set baud rate */
#define SIO_SET_DATA       4 /* Set the data characteristics of the port */

#define SIO_RESET_SIO      0
#define SIO_RESET_PURGE_RX 1
#define SIO_RESET_PURGE_TX 2

/* FTDI requests */
#define SIO_RESET_REQUEST             SIO_RESET
#define SIO_SET_BAUDRATE_REQUEST      SIO_SET_BAUD_RATE
#define SIO_SET_DATA_REQUEST          SIO_SET_DATA
#define SIO_SET_FLOW_CTRL_REQUEST     SIO_SET_FLOW_CTRL
#define SIO_SET_MODEM_CTRL_REQUEST    SIO_MODEM_CTRL
#define SIO_POLL_MODEM_STATUS_REQUEST 0x05
#define SIO_SET_EVENT_CHAR_REQUEST    0x06
#define SIO_SET_ERROR_CHAR_REQUEST    0x07
#define SIO_SET_LATENCY_TIMER_REQUEST 0x09
#define SIO_GET_LATENCY_TIMER_REQUEST 0x0A
#define SIO_SET_BITMODE_REQUEST       0x0B
#define SIO_READ_PINS_REQUEST         0x0C
#define SIO_READ_EEPROM_REQUEST       0x90
#define SIO_WRITE_EEPROM_REQUEST      0x91
#define SIO_ERASE_EEPROM_REQUEST      0x92

#define SIO_SET_DTR_MASK              0x1
#define SIO_SET_DTR_HIGH              (1 | (SIO_SET_DTR_MASK  << 8))
#define SIO_SET_DTR_LOW               (0 | (SIO_SET_DTR_MASK  << 8))
#define SIO_SET_RTS_MASK              0x2
#define SIO_SET_RTS_HIGH              (2 | (SIO_SET_RTS_MASK << 8))
#define SIO_SET_RTS_LOW               (0 | (SIO_SET_RTS_MASK << 8))

/* Whether to point stdin/stdout to ftdi_stdin/ftdi_stdout */
#define FTDI_NO_STDIO    (0 << 0)
#define FTDI_STDIO       (1 << 0)

/* Whether ftdi_putchar/ftdi_getchar are blocking */
#define FTDI_NONBLOCKING  0
#define FTDI_BLOCKING_IN  (1 << 1)
#define FTDI_BLOCKING_OUT (1 << 2)
#define FTDI_BLOCKING     (FTDI_BLOCKING_IN | FTDI_BLOCKING_OUT)

/* Initialize FTDI routines, with flags above */
void ftdi_init(int flags);

/* FILE pointers to read/write stream */
extern FILE *ftdi_stream;

/* Serial put / get */
void ftdi_putchar(uint8_t c);
int ftdi_getchar(void);
bool ftdi_can_get(void);
bool ftdi_get_dtr(void);

/* Functions used by LUFA */
void EVENT_USB_Device_ControlRequest(void);
void EVENT_USB_Device_Reset(void);
void EVENT_USB_Device_ConfigurationChanged(void);
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
				    const uint8_t wIndex,
				    const void** const DescriptorAddress)
	ATTR_WARN_UNUSED_RESULT ATTR_NON_NULL_PTR_ARG(3);

#endif

