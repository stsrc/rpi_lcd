#include "lcd_spi.h"

static int parse_input(int argc, char *argv[], struct ipc_buffer *buf)
{
	uint16_t temp;
	if (argc < 5)
		return 1;
	buf->cmd = WRITE_RECTANGLE; 
	buf->mem = malloc(1);
	if (sscanf(argv[1], "%hu", &temp) != 1)
		return 1;
	else if (sscanf(argv[2], "%hu", &buf->x) != 1)
		return 1;
	else if (sscanf(argv[3], "%hu", &buf->y) != 1)
		return 1;
	else if (sscanf(argv[4], "%hu", &buf->dx) != 1)
		return 1;
	else if (sscanf(argv[5], "%hu", &buf->dy) != 1)
		return 1;
	*buf->mem = temp;
	return 0;
}

int main(int argc, char *argv[])
{
	struct sockaddr_un server;	
	struct ipc_buffer buf;
	socklen_t len;
	int sckt;
	int ret;
	if (parse_input(argc, argv, &buf)) {
		printf("Wrong input arguments.\n");
		return 1;
	}
	server.sun_family = AF_UNIX;
	strcpy(server.sun_path, "/tmp/lcd_spi_socket\0");
	len = strlen(server.sun_path) + sizeof(server.sun_family);
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
	ret = send(sckt, buf.mem, 1, 0);
	if (ret < 0)
		perror("send");
	free(buf.mem);
	close(sckt);
	return 0;
}
