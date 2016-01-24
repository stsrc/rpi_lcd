#include "ipc_client.h"

static int ipc_send(struct ipc_buffer *buf, int mem_size) 
{
	struct sockaddr_un server;	
	socklen_t len;
	int sckt;
	int ret;

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
	ret = send(sckt, &buf->cmd, sizeof(buf->cmd), 0);
	if (ret < 0) {
		close(sckt);
		perror("send");
		return 1;
	}
	ret = send(sckt, &buf->x, 4*sizeof(uint16_t), 0);
	if (ret < 0) {
		close(sckt);
		perror("send");
		return 1;
	}
	ret = send(sckt, buf->mem, mem_size, 0);
	if (ret < 0)
		perror("send");
	close(sckt);
	return 0;
}

int ipc_send_bitmap(uint16_t x, uint16_t y, uint16_t dx, uint16_t dy,
		    uint8_t *mem)
{
	struct ipc_buffer buf;
	int ret;
	buf.x = x;
	buf.y = y;
	buf.dx = dx;
	buf.dy = dy;
	buf.cmd = WRITE_BITMAP;
	buf.mem = mem;	
	ret = ipc_send(&buf, buf.dx * buf.dy * BY_PER_PIX);	
	return ret;
}

int ipc_send_text(char *text, enum colors font, enum colors background,
		  uint16_t x, uint16_t y)
{
	int ret;
	struct ipc_buffer buf;
	buf.x = x;
	buf.y = y;
	buf.dx = strlen(text);
	buf.dy = (font << 8) | background;
	buf.cmd = WRITE_TEXT;
	buf.mem = malloc(buf.dx + 1);
	if (!buf.mem)
		return 1;
	strcpy((char *)buf.mem, text);
	ret = ipc_send(&buf, buf.dx);
	return ret;
}

int ipc_send_rectangle(uint16_t x, uint16_t y, uint16_t dx, uint16_t dy,
		       enum colors color)
{
	int ret;
	struct ipc_buffer buf;
	buf.cmd = WRITE_RECTANGLE;
	buf.mem = malloc(1);
	if (!buf.mem)
		return 1;
	*buf.mem = color;
	buf.x = x;
	buf.y = y;
	buf.dx = dx;
	buf.dy = dy;
	ret = ipc_send(&buf, 1);
	free(buf.mem);
	return ret;
}
