#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include "spidev.h"
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static void pabort(const char *s)
{
	perror(s);
	abort();
}

static const char *device = "/dev/spidev0.0";
static uint8_t mode;
static uint8_t bits = 8;
static uint32_t speed = 5000000;
static uint16_t delay = 0;

static void transfer(int fd, uint8_t data_cmd, uint8_t *tx, 
		     uint8_t *rx, uint8_t n)
{
	int ret;
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)(tx),
		.rx_buf = (unsigned long)(rx),
		.len = n,
		.delay_usecs = delay,
		.speed_hz = speed,
		.bits_per_word = bits,
		.data_cmd = data_cmd
	};
	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		pabort("ioctl");
}

static void make_byte(char *byte, uint8_t value) 
{
	for (int i = 0; i < 8; i++) {
		if (value & (1 << i))
			byte[7 - i] = '1';
		else
			byte[7 - i] = '0';
	}	
	byte[8] = '\0';
}

static void print_transfer(int data_cmd, uint8_t *tx, uint8_t *rx, uint8_t n)
{
	char byte[9];
	for (int i = 0; i < n; i++) {
		if (data_cmd) 
			printf("Transfered data: %xh. Received data: %xh.\n",
			       tx[i], rx[i]);
		else
			printf("Transfered command: %xh. Received byte: %xh.\n",
		               tx[i], rx[i]);
		make_byte(byte, rx[i]);
		printf("Received byte: %s\n", byte);
	}
}

static void lcd_reset(int fd)
{
	int data_cmd = 0;
	uint8_t tx = 0x01;
	uint8_t rx;
	transfer(fd, data_cmd, &tx, &rx, 1);		
	sleep(1);
}

static uint8_t check_clean_mem(void *tx, void *rx) 
{
	if (tx == NULL || rx == NULL) {
		if (tx != NULL)
			free(tx);
		else if (rx != NULL)
			free(rx);
		return 1;		
	}
	return 0;
}
static int transfer_wr_cmdd(int fd, int arg_cnt, ... )
{
	va_list arg_list;
	uint8_t cmd;
	uint8_t temp;
	uint8_t *tx = malloc(sizeof(uint8_t)*(arg_cnt - 1));
	uint8_t *rx = malloc(sizeof(uint8_t)*(arg_cnt - 1));
	
	if (arg_cnt != 1) {
		if (check_clean_mem(tx, rx))
			pabort("No mem.");
	}

	va_start(arg_list, arg_cnt);
	cmd = va_arg(arg_list, int);

	for (int i = 0; i < arg_cnt - 1; i++) 
		tx[i] = va_arg(arg_list, int);

	va_end(arg_list);
	
	transfer(fd, 0, &cmd, &temp, 1);
	if (arg_cnt != 1) {	
		transfer(fd, 1, tx, rx, arg_cnt - 1);
		free(tx);
		free(rx);
	}
	
	return 0;
}

int transfer_rd_d(int fd, int n, uint8_t cmd)
{
	uint8_t *tx = malloc(n);
	uint8_t *rx = malloc(n);
	if (check_clean_mem(tx, rx))
		pabort("No mem.");
	memset(tx, 0, n);
	memset(rx, 0, n);
	tx[0] = cmd;
	transfer(fd, 0, tx, rx, n);
	print_transfer(1, tx, rx, n);
	free(tx);
	free(rx);
	return 0;
}

static void lcd_init(int fd)
{
	lcd_reset(fd);
	/* Display OFF*/
	transfer_wr_cmdd(fd, 1, 0x28);
	/*Display ON*/
	transfer_wr_cmdd(fd, 1, 0x29);
	sleep(1);
	/*Display sleep out*/
	transfer_wr_cmdd(fd, 1, 0x11);
	sleep(1);
	/*Pixel format set - 18bits/pixel*/
	transfer_wr_cmdd(fd, 2, 0x3A, 0b01100110);
	/*Brightness control block on*/
	transfer_wr_cmdd(fd, 2, 0x53, 0b00101100);
	sleep(1);
	/*Display brightness - 0xff*/
	transfer_wr_cmdd(fd, 2, 0x51, 0xFF);
	sleep(1);
}

#define LENGTH_MAX 0xEF
#define HEIGHT_MAX 0x13F

static inline void lcd_create_bytes(uint16_t value, uint8_t *older, uint8_t *younger)
{
	*older = value >> 8;
	value = value << 8;
	*younger = value >> 8;	
}

static void lcd_draw(int fd, uint8_t *tx, uint8_t *rx, uint32_t mem_size)
{
	const uint8_t single_wr_max = 255;
	uint32_t written = 0;
	transfer_wr_cmdd(fd, 1, 0x2C);
	while (mem_size > single_wr_max) {
		transfer(fd, 1, &tx[written], &rx[written], single_wr_max);
		written += single_wr_max + 1;
		mem_size -= single_wr_max + 1;
	}
	if (mem_size != 0)
	transfer(fd, 1, tx, rx, mem_size);
}

static void lcd_set_rectangle(int fd, uint16_t x, uint16_t y, uint16_t height,
			      uint16_t length)
{
	uint8_t byte[4];
	lcd_create_bytes(x, &byte[0], &byte[1]);
	lcd_create_bytes(x + length, &byte[2], &byte[3]);
	transfer_wr_cmdd(fd, 5, 0x2A, byte[0], byte[1], byte[2], byte[3]);
	lcd_create_bytes(y, &byte[0], &byte[1]);
	lcd_create_bytes(y + height, &byte[2], &byte[3]);
	transfer_wr_cmdd(fd, 5, 0x2B, byte[0], byte[1], byte[2], byte[3]);
}

static int lcd_fill_rect_with_colour(uint8_t *tx, const uint32_t mem_size, 
				     const uint8_t red, const uint8_t green,
				     const uint8_t blue)
{
	const uint8_t col_max = 0x3F; //each colour may have only 6 bit value. 
	const uint8_t colors_in_pix = 3;
	if (red > col_max || green > col_max || blue > col_max)
		return 1;
	for(uint32_t i = 0; i < mem_size; i += colors_in_pix) {
		tx[i] = (blue << 2);	
		tx[i+1] = (green << 2);
		tx[i+2] = (red << 2);
	}
	return 0;
}

static int lcd_draw_rectangle(int fd, uint16_t x, uint16_t y, uint16_t height,
			      uint16_t length, uint8_t red, uint8_t green,
			      uint8_t blue)
{
	uint8_t *tx;
	uint8_t *rx;
	uint32_t mem_size;
	if (x + length > LENGTH_MAX)
	       return 1;
	else if (y + height > HEIGHT_MAX)
		return 1;
	mem_size = 3 * height * length; // 3 bytes (RGB) * pixel cnt
	tx = malloc(mem_size);
	memset(tx, 0x50 << 2, mem_size);
	rx = malloc(mem_size);//do I need it?
	if (check_clean_mem(tx, rx))
		pabort("No memory.");
	lcd_set_rectangle(fd, x, y, height, length);
	if (lcd_fill_rect_with_colour(tx, mem_size, red, green, blue)) {
		free(tx);
		free(rx);
		pabort("Wrong colours.");
	}
	lcd_draw(fd, tx, rx, mem_size);
	free(rx);
	free(tx);
	return 0;
}

int main(int argc, char *argv[])
{
	int ret = 0;
	int fd;

	fd = open(device, O_RDWR);
	if (fd < 0)
		pabort("can't open device");

	/*
	 * spi mode
	 */
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		pabort("can't set spi mode");

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		pabort("can't get spi mode");

	/*
	 * bits per word
	 */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't set bits per word");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't get bits per word");

	/*
	 * max speed hz
	 */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't set max speed hz");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't get max speed hz");

	printf("spi mode: %d\n", mode);
	printf("bits per word: %d\n", bits);
	printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);

	lcd_init(fd);
	ret = lcd_draw_rectangle(fd, 120, 160, 50, 50, 0x3F, 0x3F, 0x20);
	if (ret)
		pabort("lcd_draw_rectangle");
	close(fd);
	return ret;
}
