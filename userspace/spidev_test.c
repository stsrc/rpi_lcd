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
static uint32_t speed = 500000;
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

static int transfer_cmd_data(int fd, int arg_cnt, ... )
{
	va_list arg_list;
	uint8_t cmd;
	uint8_t temp;
	uint8_t *tx = malloc(sizeof(uint8_t)*(arg_cnt - 1));
	uint8_t *rx = malloc(sizeof(uint8_t)*(arg_cnt - 1));
	
	if (((tx == NULL) || (rx = NULL)) && (arg_cnt != 1)) {
		if (tx != NULL)
			free(tx);
		else if (rx != NULL)
			free(rx);
		return -ENOMEM;
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

static int transfer_read_data(int fd, int n, uint8_t cmd)
{
	uint8_t *tx = malloc(n);
	uint8_t *rx = malloc(n);
	if ((tx == NULL) || (rx == NULL)) {
		if (tx != NULL)
			free(tx);
		else if (rx != NULL)
			free(rx);
		return -ENOMEM;
	}
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
	transfer_cmd_data(fd, 1, 0x28);
	transfer_read_data(fd, 2, 0x0A);
	/*Display ON*/
	transfer_cmd_data(fd, 1, 0x29);
	sleep(1);
	/*Display sleep out*/
	transfer_cmd_data(fd, 1, 0x11);
	sleep(1);
	transfer_read_data(fd, 2, 0x0A);
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
	close(fd);
	return ret;
}
