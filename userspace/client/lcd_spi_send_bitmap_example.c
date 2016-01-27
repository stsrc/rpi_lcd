#include "lcd_spi.h"
#include "ipc_client.h"

static int send_bitmap(int argc, char *argv[])
{
	uint8_t red, blue, green;
	uint8_t *mem;
	uint16_t x, y, dx, dy;
	unsigned int temp;
	if (argc < 4) {
		printf("Wrong input arguments.\n");
		exit(1);
	}
	if (sscanf(argv[1], "%u", &temp) != 1) {
		printf("Wrong input arguments.\n");
		exit(1);
	}
	red = temp;
	if (sscanf(argv[2], "%u", &temp) != 1) {
		printf("Wrong input arguments.\n");
		exit(1);
	}
	green = temp;
	if (sscanf(argv[3], "%u", &temp) != 1) {
		printf("Wrong input arguments.\n");
		exit(1);
	}
	blue = temp;
	if (argc < 8) {
		x = 0;
		y = 0;
		dx = LENGTH_MAX;
		dy = HEIGHT_MAX;
	} else {
	if (sscanf(argv[4], "%hu", &x) != 1) {
		printf("Wrong input arguments.\n");
		exit(1);
	}
	else if (sscanf(argv[5], "%hu", &y) != 1) {
		printf("Wrong input arguments.\n");
		exit(1);
	}
	else if (sscanf(argv[6], "%hu", &dx) != 1) {
		printf("Wrong input arguments.\n");
		exit(1);
	}
	else if (sscanf(argv[7], "%hu", &dy) != 1) {
		printf("Wrong input arguments.\n");
		exit(1);
	}
	}
	mem = malloc(dx * dy * BY_PER_PIX);
	if (!mem)
		exit(1);
	for (int i = 0; i < dx * dy * BY_PER_PIX; i += 2) {
		mem[i] = (red << 3) | (green >> 3);
		mem[i + 1] = (green << 5) | blue;
	}
	return ipc_send_bitmap(x, y, dx, dy, mem);
}

int main(int argc, char *argv[])
{
	int ret;
	ret = send_bitmap(argc, argv);
	if (ret)
		perror("send_bitmap");
	return ret;
}
