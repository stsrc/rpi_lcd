/*
 * Copyright (C) 2006 SWAPP
 *	Andrea Paterniani <a.paterniani@swapp-eng.it>
 *
 * This program is free software;
  */

#ifndef _TOUCHPAD_SPI_H_
#define _TOUCHPAD_SPI_H_

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include <linux/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
	
#define SPI_IOC_MAGIC			'k'
#define SPI_IO_RD_CMD		_IOR(SPI_IOC_MAGIC, 8, struct lcdd_transfer)

struct lcdd_transfer {
	const uint8_t *tx_buf;
	uint8_t *rx_buf;
};

#endif /* _TOUCHPAD_SPI_H_ */
