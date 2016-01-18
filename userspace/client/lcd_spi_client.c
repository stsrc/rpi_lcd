#include "lcd_spi.h"

static int ipc_make_message(struct ipc_buffer *buf)
{
	char *text = "Raz, dwa, trzy, cztery\0";
	memset(buf, 0, sizeof(struct ipc_buffer));
	buf->cmd = WRITE_TEXT;
	buf->mem = malloc(TOT_MEM_SIZE);
	if (buf->mem == NULL)
		return 1;
	memset(buf->mem, 0, TOT_MEM_SIZE);
	strcpy((char *)buf->mem, text);
	buf->x = 0;
	buf->y = 0;
	buf->dx = strlen(text);
	buf->dy = HEIGHT_MAX;
	return 0;
}

int main(void)
{
	struct sockaddr_un server;	
	struct ipc_buffer buf;
	socklen_t len;
	int sckt;
	uint16_t temp[4];
	int ret = ipc_make_message(&buf);
	if (ret) 
		return ret;
	server.sun_family = AF_UNIX;
	strcpy(server.sun_path, "/tmp/lcd_spi_socket\0");
	len = strlen(server.sun_path) + sizeof(server.sun_family);
	sckt = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sckt < 0) {
		perror("socket");
		return ret;
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
	temp[0] = buf.x;
	temp[1] = buf.y;
	temp[2] = buf.dx;
	temp[3] = buf.dy;
	ret = send(sckt, temp, sizeof(temp), 0);
	if (ret < 0) {
		close(sckt);
		perror("send");
		return 1;
	}
	ret = send(sckt, buf.mem, buf.dx, 0);
	if (ret < 0)
		perror("send");
	free(buf.mem);
	close(sckt);
	return 0;
}