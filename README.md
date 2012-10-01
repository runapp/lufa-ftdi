FTDI serial chip emulation for LUFA
-----------------------------------

Written by Jim Paris <jim@jtan.com>

Description
-----------

This project is an example of FTDI usb-to-serial chip emulation for
LUFA.  It is fully interrupt-driven.  It acts as a FT232RL with id
0403:6001.  On Linux, the corresponding host driver is `ftdi_sio`.

Note that it's probably against the EULA of the proprietary FTDI
Windows driver to use it with non-FTDI hardware.  And using someone
else's VID will prevent you from passing USB-IF compliance tests.

Building
--------

    git submodule init
    git submodule update
    make

Configuration
-------------

Basic configuration of the FTDI emulation is in `ftdi.h`.  Read and
write buffers are maintained in a FIFO queue of length
`FTDI_TX_QUEUE_LEN` and `FTDI_RX_QUEUE_LEN`.  The latter must be
greater than the endpoint size.  For a smaller memory footprint,
`FTDI_TXRX_EPSIZE` can be reduced (try 16).

Initialization
--------------

Call `void ftdi_init(int flags)` after the corresponding LUFA
`USB_Init()`.  Valid flags are a bitwise OR of the following:

    FTDI_NO_STDIO     Don't reconfigure libc's stdio
    FTDI_STDIO        Point stdio routines to ftdi_putchar / ftdi_getchar

    FTDI_NONBLOCKING  Neither ftdi_putchar nor ftdi_getchar will block.
    FTDI_BLOCKING_IN  ftdi_getchar will block if no data is available
    FTDI_BLOCKING_OUT ftdi_putchar will block is no room in TX buffer
    FTDI_BLOCKING     Both ftdi_getchar and ftdi_putchar will block

When input is nonblocking, `ftdi_getchar` will return -1 if no data is
available.  When output is nonblocking, `ftdi_putchar` will drop data
if no TX buffer space is available.  Regardless of blocking status,
if the RX buffer is full, data received from the PC will be dropped.

License
-------

BSD 2-clause, see LICENSE
