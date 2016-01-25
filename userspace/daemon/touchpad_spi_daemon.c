#include "touchpad_spi.h"

static const char *device = "/dev/touchpad_spi";

static void transfer(int fd, uint8_t *tx, uint8_t *rx, uint32_t n, 
		     unsigned int cmd)
{
	int ret;
	struct lcdd_transfer tr = {
		.tx_buf = tx,
		.rx_buf = rx
	};
	ret = ioctl(fd, cmd, &tr);
	if (ret < 1) {
		perror("ioctl");
	
	}
}

static void print_buf(uint8_t *buf, int size)
{
	for (int i = 0; i < size; i++) 
		printf("%d: %x\n", i, buf[i]);
}

static int transfer_rd_d(int fd, uint8_t cmd, uint8_t *rx)
{
	transfer(fd, &cmd, rx, 3, SPI_IO_RD_CMD);
	return 0;
}

int switch_to_daemon(int fd)
{
	pid_t pid;
	pid = fork();
	if (pid < 0)
		exit(1);
	switch (pid) {
	case 0:
		break;
	default:
		close(fd);
		exit(0);
	}
	umask(0);
	pid = setsid();
	if (pid < 0) 
		exit(1);
	if ((chdir("/")) < 0)
		exit(1);
	close(0);
	close(1);
	close(2);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	return 0;
}






static int test(int fd)
{
	int ret;
	uint8_t rx[3];

	uint8_t cmd = generate_command(&inp);
	ret = transfer_rd_d(fd, cmd, rx);
	if (ret) {
		perror("transfer_rd_d");
		return 1;
	}
	print_buf(rx, 3);
	return 0;
}

int main(int argc, char *argv[])
{
	int fd;
	fd = open(device, O_RDWR);
	if (fd < 0) {
		perror("can't open device");
		exit(1);
	}
	while(1) {
		test(fd);
		sleep(1);
	}
	close(fd);
	return 0;
}
