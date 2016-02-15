#include "ipc_client.h"

static void convert_to_string(char *buf, uint16_t x, uint16_t y, uint16_t z)
{
	memset(buf, 0, 160);
	sprintf(buf, "x = %03hu, y = %03hu, z = %03hu", x, y, z);	
}

int main(int argc, char *argv[])
{
	uint16_t x, y, z;
	char buf[160];
	int ret;
	while(1) {
		ret = ipc_read_touchscreen(&x, &y, &z);
		if (ret) {
			perror("ipc_read_touchscreen");
			return ret;
		}
		convert_to_string(buf, x, y, z);
		ipc_send_text(buf, white, black, (48 - strlen(buf)), 0);
	}
	return 0;
}
