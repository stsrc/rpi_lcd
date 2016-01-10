/*
 * include/linux/spi/spidev.h
 *
 * Copyright (C) 2006 SWAPP
 *	Andrea Paterniani <a.paterniani@swapp-eng.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
  */

#ifndef SPIDEV_H
#define SPIDEV_H

#include <linux/types.h>

/* User space versions of kernel symbols for SPI clocking modes,
 * matching <linux/spi/spi.h>
 */

#define SPI_CPHA		0x01
#define SPI_CPOL		0x02

#define SPI_MODE_0		(0|0)
#define SPI_MODE_1		(0|SPI_CPHA)
#define SPI_MODE_2		(SPI_CPOL|0)
#define SPI_MODE_3		(SPI_CPOL|SPI_CPHA)

#define SPI_CS_HIGH		0x04
#define SPI_LSB_FIRST		0x08
#define SPI_3WIRE		0x10
#define SPI_LOOP		0x20
#define SPI_NO_CS		0x40
#define SPI_READY		0x80
#define SPI_TX_DUAL		0x100
#define SPI_TX_QUAD		0x200
#define SPI_RX_DUAL		0x400
#define SPI_RX_QUAD		0x800

/*---------------------------------------------------------------------------*/

/* IOCTL commands */

#define SPI_IOC_MAGIC			'k'

struct lcdd_transfer {
	uint32_t byte_cnt;
	uint8_t data_cmd;
	const uint8_t *tx_buf;
};	

/* not all platforms use <asm-generic/ioctl.h> or _IOC_TYPECHECK() ... */
#define SPI_MSGSIZE(N) \
	((((N)*(sizeof (struct lcdd_transfer))) < (1 << _IOC_SIZEBITS)) \
		? ((N)*(sizeof (struct lcdd_transfer))) : 0)
#define SPI_IOC_MESSAGE(N) _IOW(SPI_IOC_MAGIC, 0, char[SPI_MSGSIZE(N)])

#define SPI_IO_WR_DATA			_IOW(SPI_IOC_MAGIC, 6, struct lcdd_transfer)

#define SPI_IO_WR_CMD_DATA		_IOW(SPI_IOC_MAGIC, 7, struct lcdd_transfer)

#endif /* SPIDEV_H */