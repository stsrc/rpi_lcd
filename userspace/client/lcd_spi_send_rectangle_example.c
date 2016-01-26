#include "ipc_client.h"

static int test(int argc, char *argv[])
{
	uint16_t temp, x, y, dx, dy;
	if (argc < 5)
		return 1;
	if (sscanf(argv[1], "%hu", &temp) != 1)
		return 1;
	else if (sscanf(argv[2], "%hu", &x) != 1)
		return 1;
	else if (sscanf(argv[3], "%hu", &y) != 1)
		return 1;
	else if (sscanf(argv[4], "%hu", &dx) != 1)
		return 1;
	else if (sscanf(argv[5], "%hu", &dy) != 1)
		return 1;
	return ipc_send_rectangle(x, y, dx, dy, temp);	
}

int main(int argc, char *argv[])
{
	int ret;
	ret = test(argc, argv);
	if (ret)
		perror("test");
	return ret;	
}
