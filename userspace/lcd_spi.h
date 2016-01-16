/*
 * Copyright (C) 2006 SWAPP
 *	Andrea Paterniani <a.paterniani@swapp-eng.it>
 *
 * This program is free software;
  */

#ifndef SPIDEV_H
#define SPIDEV_H

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

#define LENGTH_MAX 240
#define HEIGHT_MAX 320
#define BY_PER_PIX 2
#define TOT_MEM_SIZE BY_PER_PIX*HEIGHT_MAX*LENGTH_MAX

struct lcdd_transfer {
	uint32_t byte_cnt;
	const uint8_t *tx_buf;
	uint8_t *rx_buf;
};	

struct ipc_buffer {
	int cmd;
	uint8_t *mem;
	uint16_t x;
	uint16_t y;
	uint16_t dx;
	uint16_t dy;
};

/*
 * ALL PROBLEMS WITH ALLIGNMENT - GET INFORMATIONS!
 */

#define SPI_IOC_MAGIC			'k'
#define SPI_IO_WR_DATA		_IOW(SPI_IOC_MAGIC, 6, struct lcdd_transfer)
#define SPI_IO_WR_CMD_DATA	_IOW(SPI_IOC_MAGIC, 7, struct lcdd_transfer)
#define SPI_IO_WR_CMD		_IOW(SPI_IOC_MAGIC, 8, struct lcdd_transfer)
#define SPI_IO_RD_CMD		_IOR(SPI_IOC_MAGIC, 8, struct lcdd_transfer)

#define WRITE_TEXT	0
#define WRITE_BITMAP	1

#endif /* SPIDEV_H */
