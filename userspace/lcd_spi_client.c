#include "lcd_spi.h"

static int ipc_make_message(struct ipc_buffer *buf)
{
	memset(buf, 0, sizeof(struct ipc_buffer));
	buf->cmd = WRITE_TEXT;
	buf->mem = malloc(TOT_MEM_SIZE);
	if (buf->mem == NULL)
		return 1;
	strcpy((char *)buf->mem, "1 2 3 4, maszeruja wyzsze sfery\n");
	return 0;
}

int main(void)
{
	struct sockaddr_un server;	
	struct ipc_buffer buf;
	socklen_t len;
	int sckt;
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
	if (ret < 0)
		perror("send");
	ret = send(sckt, buf.mem, TOT_MEM_SIZE, 0);
	if (ret < 0)
		perror("send");
	return 0;
}
