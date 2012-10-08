/*
   FTDI serial chip emulation for LUFA
   Jim Paris <jim@jtan.com>
*/

#include <LUFA/Drivers/USB/USB.h>
#include "ftdi.h"
#include "queue.h"

/* TX and RX queues */
static uint8_t ftdi_txqueue_buf[FTDI_TX_QUEUE_LEN];
static uint8_t ftdi_rxqueue_buf[FTDI_RX_QUEUE_LEN];
struct queue ftdi_txqueue =
	QUEUE_INITIALIZER(ftdi_txqueue_buf, sizeof(ftdi_txqueue_buf));
struct queue ftdi_rxqueue =
	QUEUE_INITIALIZER(ftdi_rxqueue_buf, sizeof(ftdi_rxqueue_buf));

/* Not sure if we need this, but it might make the Windows driver happy. */
static const uint8_t ftdi_eeprom[] = {
	0x00, 0x40, 0x03, 0x04, 0x01, 0x60, 0x00, 0x00,
	0xa0, 0x2d, 0x08, 0x00, 0x00, 0x00, 0x98, 0x0a,
	0xa2, 0x20, 0xc2, 0x12, 0x23, 0x10, 0x05, 0x00,
	0x0a, 0x03, 0x46, 0x00, 0x54, 0x00, 0x44, 0x00,
	0x49, 0x00, 0x20, 0x03, 0x46, 0x00, 0x54, 0x00,
	0x32, 0x00, 0x33, 0x00, 0x32, 0x00, 0x52, 0x00,
	0x20, 0x00, 0x55, 0x00, 0x53, 0x00, 0x42, 0x00,
	0x20, 0x00, 0x55, 0x00, 0x41, 0x00, 0x52, 0x00,
	0x54, 0x00, 0x12, 0x03, 0x41, 0x00, 0x36, 0x00,
	0x30, 0x00, 0x30, 0x00, 0x65, 0x00, 0x4f, 0x00,
	0x45, 0x00, 0x37, 0x00, 0x81, 0xe1, 0x35, 0x60,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9a, 0xa5
};

static struct ftdi_status {
	/* modem status */
	unsigned reserved_1:1;
	unsigned reserved_0:3;
	unsigned cts:1;
	unsigned dsr:1;
	unsigned ri:1;
	unsigned cd:1;

	/* ftdi status */
	unsigned dr:1;
	unsigned oe:1;
	unsigned pe:1;
	unsigned fe:1;
	unsigned bi:1;
	unsigned thre:1;
	unsigned temt:1;
	unsigned fifoerr:1;
} ftdi_status = { 0 };

static struct ctrl_status {
	unsigned dtr:1;
	unsigned rts:1;
} ctrl_status = { 0 };

/* Fdev compatible wrappers */
static int ftdi_fdev_write(char c, FILE *stream) {
	ftdi_putchar(c);
	return 0;
}
static int ftdi_fdev_read(FILE *stream) {
	return ftdi_getchar();
}
static FILE _ftdi_stream =
	FDEV_SETUP_STREAM(ftdi_fdev_write, ftdi_fdev_read, _FDEV_SETUP_RW);
FILE *ftdi_stream = &_ftdi_stream;

/* Init */
static bool ftdi_blocking_in = false;
static bool ftdi_blocking_out = false;
void ftdi_init(int flags)
{
	/* Reset line state */
	memset(&ftdi_status, 0, sizeof(struct ftdi_status));
	ftdi_status.reserved_1 = 1;
	ftdi_status.cts = 1;
	ftdi_status.dsr = 1;
	ftdi_status.ri = 0;
	ftdi_status.cd = 1;

	memset(&ctrl_status, 0, sizeof(struct ctrl_status));
	ctrl_status.dtr = 0;
	ctrl_status.rts = 0;

	if (flags & FTDI_STDIO) {
		stdout = ftdi_stream;
		stdin = ftdi_stream;
	}

	if (flags & FTDI_BLOCKING_IN)
		ftdi_blocking_in = true;
	if (flags & FTDI_BLOCKING_OUT)
		ftdi_blocking_out = true;
}

static inline void ftdi_control_request(void)
{
	uint16_t x;

	if (!Endpoint_IsSETUPReceived())
		return;

	if ((USB_ControlRequest.bmRequestType & CONTROL_REQTYPE_TYPE)
	    != REQTYPE_VENDOR)
		return;

	switch (USB_ControlRequest.bRequest) {

	case SIO_RESET_REQUEST:
		/* Reset */
		switch (USB_ControlRequest.wValue) {
		case SIO_RESET_SIO:
			ftdi_init(0);
			break;
		case SIO_RESET_PURGE_RX:
			while (_queue_can_get(&ftdi_rxqueue))
				(void)_queue_get(&ftdi_rxqueue);
			break;
		case SIO_RESET_PURGE_TX:
			while (_queue_can_get(&ftdi_txqueue))
				(void)_queue_get(&ftdi_txqueue);
			break;
		}
		Endpoint_ClearSETUP();
		Endpoint_ClearStatusStage();
		return;

	case SIO_READ_EEPROM_REQUEST:
		/* Return fake EEPROM data */
		x = (USB_ControlRequest.wIndex & 0xff) * 2;
		if (x + USB_ControlRequest.wLength > sizeof(ftdi_eeprom))
			return;

		Endpoint_ClearSETUP();
		Endpoint_Write_Control_Stream_LE((uint8_t *)&ftdi_eeprom[x],
						 USB_ControlRequest.wLength);
		Endpoint_ClearStatusStage();
		return;

	case SIO_WRITE_EEPROM_REQUEST:
	case SIO_ERASE_EEPROM_REQUEST:
	case SIO_SET_BAUDRATE_REQUEST:
	case SIO_SET_DATA_REQUEST:
	case SIO_SET_FLOW_CTRL_REQUEST:
	case SIO_SET_EVENT_CHAR_REQUEST:
	case SIO_SET_ERROR_CHAR_REQUEST:
	case SIO_SET_LATENCY_TIMER_REQUEST:
	case SIO_SET_BITMODE_REQUEST:
		/* Ignore these */
		Endpoint_ClearSETUP();
		Endpoint_ClearStatusStage();
		return;

	case SIO_SET_MODEM_CTRL_REQUEST:
		if (USB_ControlRequest.wValue & SIO_SET_DTR_HIGH)
			ctrl_status.dtr = 1;
		else if (USB_ControlRequest.wValue & SIO_SET_DTR_LOW)
			ctrl_status.dtr = 0;
		if (USB_ControlRequest.wValue & SIO_SET_RTS_HIGH)
			ctrl_status.rts = 1;
		else if (USB_ControlRequest.wValue & SIO_SET_RTS_LOW)
			ctrl_status.rts = 0;
		Endpoint_ClearSETUP();
		Endpoint_ClearStatusStage();
		return;

	case SIO_GET_LATENCY_TIMER_REQUEST:
		/* Return dummy value */
		if (USB_ControlRequest.wLength != 1)
			return;
		Endpoint_ClearSETUP();
		x = 100;
		Endpoint_Write_Control_Stream_LE((uint8_t *)&x, 1);
		Endpoint_ClearStatusStage();
		return;

	case SIO_READ_PINS_REQUEST:
		/* Return dummy value */
		if (USB_ControlRequest.wLength != 1)
			return;
		Endpoint_ClearSETUP();
		x = 0;
		Endpoint_Write_Control_Stream_LE((uint8_t *)&x, 1);
		Endpoint_ClearStatusStage();
		return;

	case SIO_POLL_MODEM_STATUS_REQUEST:
		/* Send status */
		if (USB_ControlRequest.wLength != 2)
			return;
		Endpoint_ClearSETUP();
		Endpoint_Write_Control_Stream_LE((uint8_t *)&ftdi_status, 2);
		Endpoint_ClearStatusStage();
		return;
	}
}

/* Device descriptor */
static const USB_Descriptor_Device_t PROGMEM DeviceDescriptor =
{
	.Header = {
		.Size = sizeof(USB_Descriptor_Device_t),
		.Type = DTYPE_Device
	},

	.USBSpecification = VERSION_BCD(02.00),
	.Class	    = 0x00,
	.SubClass	 = 0x00,
	.Protocol	 = 0x00,

	.Endpoint0Size    = FIXED_CONTROL_ENDPOINT_SIZE,

	.VendorID	 = 0x0403,  /* FTDI */
	.ProductID	= 0x6001,  /* FT232 */
	.ReleaseNumber	  = 0x0600,  /* FT232 */

	.ManufacturerStrIndex   = 0x01,
	.ProductStrIndex	= 0x02,
	.SerialNumStrIndex      = USE_INTERNAL_SERIAL,

	.NumberOfConfigurations = FIXED_NUM_CONFIGURATIONS
};

/* Configuration descriptor */
typedef struct
{
	USB_Descriptor_Configuration_Header_t Config;
	USB_Descriptor_Interface_t	      FTDI_Interface;
	USB_Descriptor_Endpoint_t             FTDI_DataInEndpoint;
	USB_Descriptor_Endpoint_t             FTDI_DataOutEndpoint;
} USB_Descriptor_Config_t;

static const USB_Descriptor_Config_t PROGMEM ConfigurationDescriptor =
{
	.Config = {
		.Header = {
			.Size = sizeof(USB_Descriptor_Configuration_Header_t),
			.Type = DTYPE_Configuration
		},

		.TotalConfigurationSize = sizeof(USB_Descriptor_Config_t),
		.TotalInterfaces	= 1,

		.ConfigurationNumber    = 1,
		.ConfigurationStrIndex  = 0,
		.ConfigAttributes       = USB_CONFIG_ATTR_RESERVED,
		.MaxPowerConsumption    = USB_CONFIG_POWER_MA(100)
	},

	.FTDI_Interface = {
		.Header	= {
			.Size = sizeof(USB_Descriptor_Interface_t),
			.Type = DTYPE_Interface
		},
		.InterfaceNumber  = 0,
		.AlternateSetting = 0,
		.TotalEndpoints	  = 2,
		.Class    = 0xff,
		.SubClass = 0xff,
		.Protocol = 0xff,
		.InterfaceStrIndex = 0x02,  /* Reuse product string */
	},

	.FTDI_DataInEndpoint = {
		.Header = {
			.Size = sizeof(USB_Descriptor_Endpoint_t),
			.Type = DTYPE_Endpoint
		},
		.EndpointAddress = FTDI_TX_EPADDR,
		.Attributes	 = EP_TYPE_BULK,
		.EndpointSize	 = FTDI_TXRX_EPSIZE,
		.PollingIntervalMS = 0x00,
	},

	.FTDI_DataOutEndpoint = {
		.Header = {
			.Size = sizeof(USB_Descriptor_Endpoint_t),
			.Type = DTYPE_Endpoint
		},
		.EndpointAddress = FTDI_RX_EPADDR,
		.Attributes	 = EP_TYPE_BULK,
		.EndpointSize	 = FTDI_TXRX_EPSIZE,
		.PollingIntervalMS = 0x00,
	},
};

/* String descriptors */
#define STRING_DESC(s) { { (sizeof(s)-2)+1+1, DTYPE_String }, s }
static const USB_Descriptor_String_t PROGMEM string0 = {
	{ USB_STRING_LEN(1), DTYPE_String }, { LANGUAGE_ID_ENG }
};
static const USB_Descriptor_String_t PROGMEM string1 = STRING_DESC(L"LUFA");
static const USB_Descriptor_String_t PROGMEM string2 = STRING_DESC(L"FT232");
static const USB_Descriptor_String_t *strings[] = {
	&string0,
	&string1,
	&string2
};

uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
				    const uint8_t wIndex,
				    const void** const DescriptorAddress)
{
	const uint8_t DescriptorType   = (wValue >> 8);
	const uint8_t DescriptorNumber = (wValue & 0xFF);

	const void* Address = NULL;
	uint16_t Size = 0;

	switch (DescriptorType)
	{
		case DTYPE_Device:
			Address = &DeviceDescriptor;
			Size    = sizeof(USB_Descriptor_Device_t);
			break;
		case DTYPE_Configuration:
			Address = &ConfigurationDescriptor;
			Size    = sizeof(USB_Descriptor_Config_t);
			break;
		case DTYPE_String:
			if (DescriptorNumber >= (sizeof(strings) /
						 sizeof(strings[0])))
				break;
			Address = strings[DescriptorNumber];
			Size = pgm_read_byte(
				&strings[DescriptorNumber]->Header.Size);
			break;
	}

	*DescriptorAddress = Address;
	return Size;
}

/* LUFA events and USB interrupt handlers */

void EVENT_USB_Device_ControlRequest(void)
{
	ftdi_control_request();
}

void EVENT_USB_Device_Reset(void)
{
	Endpoint_SelectEndpoint(ENDPOINT_CONTROLEP);
	USB_INT_Enable(USB_INT_RXSTPI);
}

void EVENT_USB_Device_ConfigurationChanged(void)
{
	uint8_t PrevSelectedEndpoint = Endpoint_GetCurrentEndpoint();

	/* Configure endpoints */

	/* IN */
	Endpoint_ConfigureEndpoint(FTDI_TX_EPADDR,
				   EP_TYPE_BULK, FTDI_TXRX_EPSIZE, 2);
	Endpoint_SelectEndpoint(FTDI_TX_EPADDR);
	UEIENX = (1 << TXINE);

	/* OUT */
	Endpoint_ConfigureEndpoint(FTDI_RX_EPADDR,
				   EP_TYPE_BULK, FTDI_TXRX_EPSIZE, 2);
	Endpoint_SelectEndpoint(FTDI_RX_EPADDR);
	UEIENX = (1 << RXOUTE);

	/* Done */
	Endpoint_SelectEndpoint(PrevSelectedEndpoint);
}

static inline void ftdi_bulk_in(void)
{
	int len, i;

	/* Data format: ftdi_status structure (2 bytes) followed by the
	   serial data. */
	len = _queue_get_len(&ftdi_txqueue);

	if (len >= FTDI_TXRX_EPSIZE - 2) {
		/* more to send */
		ftdi_status.thre = 0;
		ftdi_status.temt = 0;
		len = FTDI_TXRX_EPSIZE - 2;
	} else {
		/* transmitter empty */
		ftdi_status.thre = 0;
		ftdi_status.temt = 0;
	}

	Endpoint_Write_8(((uint8_t *)&ftdi_status)[0]);
	Endpoint_Write_8(((uint8_t *)&ftdi_status)[1]);

	for (i = 0; i < len; i++)
		Endpoint_Write_8(_queue_get(&ftdi_txqueue));

	/* If that packet was empty, disable the IN interrupt.  We'll
	   reenable it when we have more data to send. */
	if (len == 0) {
		UEIENX &= ~(1 << TXINE);
	}

	/* Send it */
	Endpoint_ClearIN();
}

static inline void ftdi_bulk_out(void)
{
	int i;
	int len;

	len = Endpoint_BytesInEndpoint();

	if (_queue_get_free(&ftdi_rxqueue) < len) {
		/* We can't deal with this right now.  Disable OUT
		   interrupt; we'll reenable after reading some bytes */
		UEIENX &= ~(1 << RXOUTE);
		return;
	}

	for (i = 0; i < len; i++) {
		/* This will discard incoming bytes if queue is full */
		_queue_put(&ftdi_rxqueue, Endpoint_Read_8());
	}
	Endpoint_ClearOUT();
}

ISR(USB_COM_vect, ISR_BLOCK)
{
	uint8_t PrevSelectedEndpoint = Endpoint_GetCurrentEndpoint();

	if (UEINT & (1 << ENDPOINT_CONTROLEP)) {
		Endpoint_SelectEndpoint(ENDPOINT_CONTROLEP);

		if (Endpoint_IsSETUPReceived())
			USB_Device_ProcessControlRequest();
	}

	if (UEINT & (1 << (FTDI_TX_EPADDR & ENDPOINT_EPNUM_MASK))) {
		Endpoint_SelectEndpoint(FTDI_TX_EPADDR);

		if (Endpoint_IsINReady()) {
			/* Handle IN (device-to-host) */
			ftdi_bulk_in();
		}
	}

	if (UEINT & (1 << (FTDI_RX_EPADDR & ENDPOINT_EPNUM_MASK))) {
		Endpoint_SelectEndpoint(FTDI_RX_EPADDR);

		if (Endpoint_IsOUTReceived()) {
			/* Handle OUT (host-to-device) */
			ftdi_bulk_out();
		}
	}

	Endpoint_SelectEndpoint(PrevSelectedEndpoint);
}

/* Get / put routines */

void ftdi_putchar(uint8_t c)
{
	/* Block until we can enqueue it, or drop if nonblocking */
	while (queue_put(&ftdi_txqueue, c) == -1)
		if (ftdi_blocking_out == false)
			return;

	/* Enable the IN endpoint interrupt, since we have data to send. */
	if (USB_DeviceState == DEVICE_STATE_Configured) {
		Endpoint_SelectEndpoint(FTDI_TX_EPADDR);
		UEIENX |= (1 << TXINE);
	}
}

int ftdi_getchar(void)
{
	int ret;

	/* Block until byte is ready, or return 0 if nonblocking */
	while ((ret = queue_get(&ftdi_rxqueue)) < 0)
		if (ftdi_blocking_in == false)
			return -1;

	/* Reenable the OUT endpoint interrupt, since we read some data */
	if (USB_DeviceState == DEVICE_STATE_Configured) {
		Endpoint_SelectEndpoint(FTDI_RX_EPADDR);
		UEIENX |= (1 << RXOUTE);
	}

	return ret;
}

bool ftdi_can_get(void)
{
	return queue_can_get(&ftdi_rxqueue);
}

bool ftdi_get_dtr(void)
{
	return ctrl_status.dtr;
}
