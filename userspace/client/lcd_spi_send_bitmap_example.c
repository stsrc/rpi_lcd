#include "lcd_spi.h"

static int ipc_make_message(struct ipc_buffer *buf, uint8_t red, 
			    uint8_t green, uint8_t blue)
{
	buf->cmd = WRITE_BITMAP;
	buf->mem = malloc(buf->dx * buf->dy * BY_PER_PIX);
	if (!buf->mem)
		return 1;
	for (int i = 0; i < buf->dx * buf->dy * BY_PER_PIX; i += 2) {
		buf->mem[i] = red << 3 | ((green & 0b00111000) >> 3);
		buf->mem[i+1] = ((green & 0b00000111) << 5) | blue;
	}
	return 0;
}

static int parse_input(int argc, char *argv[], uint8_t *red, uint8_t *green,
		       uint8_t *blue, struct ipc_buffer *buf)
{
	unsigned int temp;
	if (argc < 4)
		return 1;
	if (sscanf(argv[1], "%u", &temp) != 1)
		return 1;
	*red = (uint8_t)temp;
	if (sscanf(argv[2], "%u", &temp) != 1)
		return 1;
	*green = (uint8_t)temp;
	if (sscanf(argv[3], "%u", &temp) != 1)
		return 1;
	*blue = (uint8_t)temp;
	if (argc < 8) {
		buf->x = 0;
		buf->y = 0;
		buf->dx = LENGTH_MAX;
		buf->dy = HEIGHT_MAX;
		return 0;
	}
	if (sscanf(argv[4], "%hu", &buf->x) != 1)
		return 1;
	else if (sscanf(argv[5], "%hu", &buf->y) != 1)
		return 1;
	else if (sscanf(argv[6], "%hu", &buf->dx) != 1)
		return 1;
	else if (sscanf(argv[7], "%hu", &buf->dy) != 1)
		return 1;
	return 0;
}

int main(int argc, char *argv[])
{
	struct sockaddr_un server;	
	struct ipc_buffer buf;
	socklen_t len;
	int sckt;
	int ret;
	uint8_t red, blue, green;
	if (parse_input(argc, argv, &red, &green, &blue, &buf)) {
		printf("Wrong input arguments.\n");
		return 1;
	}
	server.sun_family = AF_UNIX;
	strcpy(server.sun_path, "/tmp/lcd_spi_socket\0");
	len = strlen(server.sun_path) + sizeof(server.sun_family);
	if (ipc_make_message(&buf, red, green, blue)) {
		perror("ipc_make_message");
		return 1;
	}
	sckt = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sckt < 0) {
		perror("socket");
		return 1;
	}
	ret = connect(sckt, (struct sockaddr *)&server, len);
	if (ret) {
		close(sckt);
		perror("connect");
		return ret;
	}
	ret = send(sckt, &buf.cmd, sizeof(buf.cmd), 0);
	if (ret < 0) {
		close(sckt);
		perror("send");
		return 1;
	}
	ret = send(sckt, &buf.x, 4*sizeof(uint16_t), 0);
	if (ret < 0) {
		close(sckt);
		perror("send");
		return 1;
	}
	ret = send(sckt, buf.mem, buf.dx * buf.dy * BY_PER_PIX, 0);
	if (ret < 0)
		perror("send");
	free(buf.mem);
	close(sckt);
	return 0;
}
