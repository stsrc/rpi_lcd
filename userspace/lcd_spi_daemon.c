#include "lcd_spi.h"

static void pabort(const char *s)
{
	perror(s);
	abort();
}

static const char *device = "/dev/lcd_spi";

static void transfer(int fd, uint8_t *tx, 
		     uint8_t *rx, uint32_t n, unsigned int
		     cmd)
{
	int ret;
	struct lcdd_transfer tr = {
		.byte_cnt = n,
		.tx_buf = tx,
		.rx_buf = rx
	};
	ret = ioctl(fd, cmd, &tr);
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

static void print_transfer(int data_cmd, uint8_t *tx, uint8_t *rx, uint32_t n)
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

static uint8_t check_clean_mem(void *tx, void *rx, int check_rx) 
{
	if (tx == NULL || (check_rx && rx == NULL)) {
		if (tx != NULL)
			free(tx);
		else if (rx != NULL)
			free(rx);
		return 1;		
	}
	return 0;
}

static inline int transfer_wr_cmd(int fd, uint8_t cmd)
{
	transfer(fd, &cmd, NULL, 1, SPI_IO_WR_CMD);
	return 0;
}

static int transfer_wr_cmd_data(int fd, int arg_cnt, ... )
{
	va_list arg_list;
	uint8_t cmd;
	uint8_t *tx = NULL;
	
	va_start(arg_list, arg_cnt);
	cmd = va_arg(arg_list, int);

	if (arg_cnt == 1) {
		transfer(fd, &cmd, NULL, 1, SPI_IO_WR_CMD);
		va_end(arg_list);
		return 0;
	}

	tx = malloc(sizeof(uint8_t)*(arg_cnt));
	if (check_clean_mem(tx, NULL, 0))
		pabort("No mem.");
	tx[0] = cmd;
	for (int i = 1; i < arg_cnt; i++) 
		tx[i] = va_arg(arg_list, int);
	
	va_end(arg_list);
	transfer(fd, tx, NULL, arg_cnt, SPI_IO_WR_CMD_DATA);
	free(tx);
	return 0;
}

int transfer_rd_d(int fd, int n, uint8_t cmd)
{
	uint8_t *tx = malloc(n);
	uint8_t *rx = malloc(n);
	if (check_clean_mem(tx, rx, 1))
		pabort("No mem.");
	memset(tx, 0xFF, n);
	memset(rx, 0, n);
	tx[0] = cmd;
	transfer(fd, tx, rx, n, SPI_IO_RD_CMD);
	print_transfer(1, tx, rx, n);
	free(tx);
	free(rx);
	return 0;
}

static void lcd_init(int fd)
{
	struct timespec req;
	/*Reset*/
	transfer_wr_cmd(fd, 0x01);
	req.tv_sec = 0;
	req.tv_nsec = 120000000; //120ms.	
	nanosleep(&req, NULL);
	/*Display ON*/
	transfer_wr_cmd(fd, 0x29);
	/*Display sleep out*/
	transfer_wr_cmd(fd, 0x11);
	req.tv_nsec = 5000000; //5ms
	nanosleep(&req, NULL);
	/*Pixel format set - 16bits/pixel*/
	transfer_wr_cmd_data(fd, 2, 0x3A, 0x55);
	/*RGB-BGR Order*/
	transfer_wr_cmd_data(fd, 2, 0x36, 0b00001000);
	req.tv_nsec = 120000000;
	nanosleep(&req, NULL);
	/*Brightness control block on*/
	transfer_wr_cmd_data(fd, 2, 0x53, 0x2C);
	nanosleep(&req, NULL); //120ms
	/*Display brightness - 0xff*/
	transfer_wr_cmd_data(fd, 2, 0x51, 0x12);
	nanosleep(&req, NULL); //120ms
}

static inline void lcd_create_bytes(uint16_t value, uint8_t *older, uint8_t *younger)
{
	*older = value >> 8;
	value = value << 8;
	*younger = value >> 8;	
}

static void lcd_draw(int fd, uint8_t *tx, uint8_t *rx, uint32_t mem_size)
{
	const uint32_t single_wr_max = TOT_MEM_SIZE;
	uint32_t written = 0;
	transfer_wr_cmd(fd, 0x2C);
	while (mem_size >= single_wr_max) {
		transfer(fd, &tx[written], NULL, single_wr_max, 
			 SPI_IO_WR_DATA);
		written += single_wr_max;
		mem_size -= single_wr_max;
	}
	if (mem_size != 0)
	transfer(fd, &tx[written], NULL, mem_size, SPI_IO_WR_DATA);
}

static void lcd_set_rectangle(int fd, uint16_t x, uint16_t y, uint16_t height,
			      uint16_t length)
{
	uint8_t byte[4];
	lcd_create_bytes(x, &byte[0], &byte[1]);
	lcd_create_bytes(x + length, &byte[2], &byte[3]);
	transfer_wr_cmd_data(fd, 5, 0x2A, byte[0], byte[1], byte[2], byte[3]);
	lcd_create_bytes(y, &byte[0], &byte[1]);
	lcd_create_bytes(y + height, &byte[2], &byte[3]);
	transfer_wr_cmd_data(fd, 5, 0x2B, byte[0], byte[1], byte[2], byte[3]);
}

static int lcd_colour_test(const uint8_t red, const uint8_t green, const 
			   uint8_t blue)
{
	const uint8_t rb_max = 31;
	const uint8_t g_max = 63;

	if ((red > rb_max) || (blue > rb_max))
		return 1;
	else if (green > g_max)
		return 1;
	else
		return 0;
}

static inline void lcd_colour_prepare(const uint8_t red, const uint8_t green, 
				      const uint8_t blue, uint8_t *tx)
{
	uint8_t first = 0;
	uint8_t second = 0;
	first = red << 3;
	first |= ((green & 0b00111000) >> 3);
	second = ((green & 0b00000111) << 5);
	second |= blue;
	*tx = first;
	*(tx + 1) = second;
}

static int lcd_fill_rect_with_colour(uint8_t *tx, const uint32_t mem_size, 
				     const uint8_t red, const uint8_t green,
				     const uint8_t blue)
{
	if (lcd_colour_test(red, green, blue))
		return 1;
	for (uint32_t i = 0; i < mem_size; i += BY_PER_PIX) 
		lcd_colour_prepare(red, green, blue, &tx[i]);
	return 0;
}

static int lcd_draw_rectangle(int fd, uint16_t x, uint16_t y, uint16_t length,
			      uint16_t height, uint8_t red, uint8_t green,
			      uint8_t blue)
{
	uint8_t *tx;
	uint8_t *rx = NULL;
	uint32_t mem_size;
	if (x + length > LENGTH_MAX)
		pabort("Wrong parameters");
	else if (y + height > HEIGHT_MAX)
		pabort("Wrong parameters");
	mem_size = BY_PER_PIX * height * length;
	tx = malloc(mem_size);
	memset(tx, 0, mem_size);
	if (check_clean_mem(tx, rx, 0))
		pabort("No memory.");
	lcd_set_rectangle(fd, x, y, height, length);
	if (lcd_fill_rect_with_colour(tx, mem_size, red, green, blue)) {
		free(tx);
		free(rx);
		pabort("Wrong colours.");
	}
	lcd_draw(fd, tx, rx, mem_size);
	free(rx);
	free(tx);
	return 0;
}

static int lcd_draw_bitmap(int fd, struct ipc_buffer *buf)
{
	lcd_draw_rectangle(fd, 0, 0, 100, 100, 20, 0, 0);
	return 0;
}

static inline int ipc_make_socket()
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

static int ipc_make(int *socket, struct sockaddr_un *local) {
	int ret;
	ipc_prepare_struct(local);
	*socket = ipc_make_socket();
	if (*socket < 0)
		return *socket;
	ret = ipc_bind_socket(local, *socket);
	return ret;
}

static int ipc_accept(int socket, struct sockaddr_un *local, 
	       struct sockaddr_un *connected) {
	int ret;
	socklen_t len = sizeof(struct sockaddr_un);
	ret = listen(socket, 2);
	if (ret)
		return ret;
	return accept(socket, (struct sockaddr *)connected, &len);
}

static int ipc_write_text(int socket, struct sockaddr_un *connected, 
			  struct ipc_buffer *buf)
{
	int ret = recv(socket, buf->mem, TOT_MEM_SIZE, 0);
	printf("text: %s\n", (char *)buf->mem);
	ret = 0;
	return ret;
}

static int ipc_draw_bitmap(int fd, int socket, struct sockaddr_un *connected, 
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
	cnt = buf->dx*buf->dy;
	ret = recv(socket, buf->mem, cnt, MSG_WAITALL);
	if (ret != cnt)
		return 1;
	return lcd_draw_bitmap(fd, buf);
}

static int ipc_action(int fd, int socket, struct sockaddr_un *connected, 
		     struct ipc_buffer *buf) 
{
	int ret;
	ret = recv(socket, &(buf->cmd), sizeof(buf->cmd), MSG_WAITALL);
	if (ret != sizeof(buf->cmd))
		return 1;
	switch(buf->cmd) {
	case WRITE_TEXT:
		return ipc_write_text(socket, connected, buf);
	case PRINT:
		return ipc_draw_bitmap(fd, socket, connected, buf);
	}
	return 1;
}

static int ipc_main(int fd)
{
	struct sockaddr_un server, client;
	int ret, server_socket, client_socket;
	struct ipc_buffer buf;
	buf.mem = malloc(TOT_MEM_SIZE);
	if (!buf.mem)	
		return 1;
	ret = ipc_make(&server_socket, &server);
	if (ret)
		goto err;
	while(1) {
		client_socket = ipc_accept(server_socket, &server, &client);
		if (client_socket < 0)
			goto err;
		ret = ipc_action(fd, client_socket, &client, &buf);
		if (ret)
			goto err;
		close(client_socket);
	}
err:
	perror("ipc_main");
	close(server_socket);
	return ret;
}

int lcd_clear_background(int fd) 
{
	return lcd_draw_rectangle(fd, 0, 0, LENGTH_MAX, HEIGHT_MAX, 15, 
				  63, 0);
}

int main(int argc, char *argv[])
{
	int ret = 0;
	int fd;
	fd = open(device, O_RDWR);
	if (fd < 0)
		pabort("can't open device");
	lcd_init(fd);
	lcd_clear_background(fd);
	ipc_main(fd);
	close(fd);
	return ret;
}
