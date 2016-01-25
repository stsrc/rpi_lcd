/*
 * Copyright (C) 2006 SWAPP
 *	Andrea Paterniani <a.paterniani@swapp-eng.it>
 *
 * This program is free software;
  */

#ifndef _LCD_SPI_H_
#define _LCD_SPI_H_

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

#include "ipc.h"

#define LENGTH_MAX 240
#define HEIGHT_MAX 320
#define BY_PER_PIX 2
#define TOT_MEM_SIZE BY_PER_PIX * HEIGHT_MAX * LENGTH_MAX
	
#define SPI_IOC_MAGIC			'k'
#define SPI_IO_WR_DATA		_IOW(SPI_IOC_MAGIC, 6, struct lcdd_transfer)
#define SPI_IO_WR_CMD		_IOW(SPI_IOC_MAGIC, 7, struct lcdd_transfer)
#define SPI_IO_RD_CMD		_IOR(SPI_IOC_MAGIC, 7, struct lcdd_transfer)

enum colors {
	black, white, red, blue, yellow, green, brown, background 
};

struct lcdd_transfer {
	uint32_t byte_cnt;
	const uint8_t *tx_buf;
	uint8_t *rx_buf;
};

int lcd_draw_bitmap(int fd, struct ipc_buffer *buf);
int lcd_draw_text(int fd, struct ipc_buffer *buf);
void lcd_return_colors(enum colors color, uint8_t *red, uint8_t *green,
		       uint8_t *blue);
int lcd_draw_rectangle(int fd, uint16_t x, uint16_t y, uint16_t length,
		       uint16_t height, uint8_t red, uint8_t green, 
		       uint8_t blue);
int lcd_read_touchscreen(int fd, uint16_t *x, uint16_t *y, uint16_t *z);
#endif /* _LCD_SPI_H_ */
