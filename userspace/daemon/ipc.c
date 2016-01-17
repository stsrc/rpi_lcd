#include "ipc.h"

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

static inline int ipc_write_text(int socket, struct sockaddr_un *connected, 
				 struct ipc_buffer *buf)
{
	int ret = recv(socket, buf->mem, TOT_MEM_SIZE, 0);
	ret = 0;
	return ret;
}

static inline int ipc_draw_bitmap(int fd, int socket, 
				  struct sockaddr_un *connected, 
				  struct ipc_buffer *buf)
{
	uint16_t temp[4];
	int cnt = sizeof(temp);
	int ret = recv(socket, temp, cnt, MSG_WAITALL);
	if (ret != cnt)
		return 1;
	buf->x = temp[0];
	buf->y = temp[1];
	buf->dx = temp[2];
	buf->dy = temp[3];
	cnt = BY_PER_PIX*buf->dx*buf->dy;
	ret = recv(socket, buf->mem, cnt, MSG_WAITALL);
	if (ret != cnt)
		return 1;
	return lcd_draw_bitmap(fd, buf);
}

static inline int ipc_action(int fd, int socket, struct sockaddr_un *connected, 
			     struct ipc_buffer *buf) 
{
	int ret;
	ret = recv(socket, &(buf->cmd), sizeof(buf->cmd), MSG_WAITALL);
	if (ret != sizeof(buf->cmd)) {
		printf("Bug\n");
		return 1;
	}
	switch(buf->cmd) {
	case WRITE_TEXT:
		return ipc_write_text(socket, connected, buf);
	case WRITE_BITMAP:
		return ipc_draw_bitmap(fd, socket, connected, buf);
	}
	return 1;
}

int ipc_main(int fd)
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
		goto err;
	}
	while(1) {
		client_socket = ipc_accept(server_socket, &server, &client);
		if (client_socket < 0) {
			perror("ipc_accept");
			goto err;
		}
		ret = ipc_action(fd, client_socket, &client, &buf);
		if (ret) {
			perror("ipc_action");
			goto err;
		}
		close(client_socket);
	}
	close(server_socket);
	return 0;
err:
	close(client_socket);
	close(server_socket);
	return ret;
}
