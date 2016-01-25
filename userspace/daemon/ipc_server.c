#include "ipc_server.h"

static inline int ipc_make_socket(void)
{
	return socket(AF_UNIX, SOCK_STREAM, 0);
}

static inline int ipc_bind_socket(struct sockaddr_un *address, int socket)
{
	socklen_t len = strlen(address->sun_path) + sizeof(address->sun_family);
	return bind(socket, (struct sockaddr *)address, len);	
}

static inline void ipc_prepare_struct(struct sockaddr_un *local)
{
	local->sun_family = AF_UNIX;
	strcpy(local->sun_path, "/tmp/lcd_spi_socket\0");
	unlink(local->sun_path);
}

static inline int ipc_make(int *socket, struct sockaddr_un *local) {
	int ret;
	ipc_prepare_struct(local);
	*socket = ipc_make_socket();
	if (*socket < 0)
		return *socket;
	ret = ipc_bind_socket(local, *socket);
	return ret;
}

static inline int ipc_accept(int socket, struct sockaddr_un *local, 
			     struct sockaddr_un *connected) {
	int ret;
	socklen_t len = sizeof(struct sockaddr_un);
	ret = listen(socket, 2);
	if (ret)
		return ret;
	return accept(socket, (struct sockaddr *)connected, &len);
}

static inline int ipc_write_text(int fd, int socket, 
				 struct sockaddr_un *connected, 
				 struct ipc_buffer *buf)
{
	int ret;
	uint16_t cnt = 4 * sizeof(uint16_t);
	ret = recv(socket, &buf->x, cnt, MSG_WAITALL);
	if (ret != cnt)
		return 1;
	cnt = buf->dx;
	ret = recv(socket, buf->mem, cnt, MSG_WAITALL);
	if (ret != cnt)
		return 1;
	return lcd_draw_text(fd, buf);
}

static inline int ipc_draw_bitmap(int fd, int socket, 
				  struct sockaddr_un *connected, 
				  struct ipc_buffer *buf)
{
	int cnt = 4 * sizeof(uint16_t);
	int ret = recv(socket, &buf->x, cnt, MSG_WAITALL);
	if (ret != cnt)
		return 1;
	cnt = BY_PER_PIX*buf->dx*buf->dy;
	ret = recv(socket, buf->mem, cnt, MSG_WAITALL);
	if (ret != cnt)
		return 1;
	return lcd_draw_bitmap(fd, buf);
}

static inline int ipc_draw_rectangle(int fd, int socket, 
				     struct sockaddr_un *connected,
				     struct ipc_buffer *buf)
{

	uint8_t red, green, blue;
	int cnt = 4 * sizeof(uint16_t);
	int ret = recv(socket, &buf->x, cnt, MSG_WAITALL);
	if (ret != cnt)
		return 1;
	cnt = 1;
	ret = recv(socket, buf->mem, cnt, MSG_WAITALL);
	if (ret != cnt)
		return 1;
	lcd_return_colors(*buf->mem, &red, &green, &blue);
	ret = lcd_draw_rectangle(fd, buf->x, buf->y, buf->dx, buf->dy, red,
				 green, blue);
	return ret;
}

static inline int ipc_read_touchscreen(int fd, int socket, 
				       struct sockaddr_un *connected)
{
	int ret;
	uint16_t x, y, z;
	ret = lcd_read_touchscreen(fd, &x, &y, &z);
	if (ret)
		return 1;
	send(socket, x, 2, 0);
	send(socket, y, 2, 0);
	send(socket, z, 2, 0);
	return 0;
}

static inline int ipc_action(int fd_lcd, int fd_touch, int socket, 
			     struct sockaddr_un *connected, 
			     struct ipc_buffer *buf) 
{
	int ret;
	ret = recv(socket, &(buf->cmd), sizeof(buf->cmd), MSG_WAITALL);
	if (ret != sizeof(buf->cmd))
		return 1;
	switch(buf->cmd) {
	case WRITE_TEXT:
		return ipc_write_text(fd_lcd, socket, connected, buf);
	case WRITE_BITMAP:
		return ipc_draw_bitmap(fd_lcd, socket, connected, buf);
	case WRITE_RECTANGLE:
		return ipc_draw_rectangle(fd_lcd, socket, connected, buf);
	case READ_TOUCHSCREEN:
		return ipc_read_touchscreen(fd_touch, socket, connected);
	}
	return 1;
}

int ipc_main(int fd_lcd, int fd_touch)
{
	struct sockaddr_un server, client;
	int ret, server_socket, client_socket;
	struct ipc_buffer buf;
	buf.mem = malloc(TOT_MEM_SIZE);
	if (!buf.mem) {	
		perror("malloc");
		return 1;
	}
	ret = ipc_make(&server_socket, &server);
	if (ret) {
		perror("ipc_make");
		return 1;
	}
	while(1) {
		client_socket = ipc_accept(server_socket, &server, &client);
		if (client_socket < 0) 
			perror("ipc_accept");
		ret = ipc_action(fd_lcd, fd_touch, client_socket, &client, &buf);
		if (ret)
			perror("ipc_action");
		close(client_socket);
	}
	close(server_socket);
	return 0;
}
