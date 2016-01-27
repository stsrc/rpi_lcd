#include "ipc_client.h"

static int write_rectangle(int argc, char *argv[])
{
	uint16_t temp, x, y, dx, dy;
	if (argc < 2) {
		printf("Wrong input arguments.\n");
		exit(1);
	}
	if (sscanf(argv[1], "%hu", &temp) != 1) {
		printf("Wrong input argument.\n");
		exit(1);
	}
	if (argc == 6) { 
		if (sscanf(argv[2], "%hu", &x) != 1)
			exit(1);
		else if (sscanf(argv[3], "%hu", &y) != 1)
			exit(1);
		else if (sscanf(argv[4], "%hu", &dx) != 1)
			exit(1);
		else if (sscanf(argv[5], "%hu", &dy) != 1)
			exit(1);
	} else {
		x = 0;
		y = 0;
		dx = 240;
		dy = 320;
	}
	return ipc_send_rectangle(x, y, dx, dy, temp);	
}

int main(int argc, char *argv[])
{
	int ret;
	ret = write_rectangle(argc, argv);
	if (ret)
		perror("write_rectangle");
	return ret;	
}
