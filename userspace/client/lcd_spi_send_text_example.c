#include "ipc_client.h"

int main(int argc, char *argv[])
{
	int ret;
	uint16_t arg[2];
	if (argc == 4) {
		if (sscanf(argv[2], "%hu", &arg[0]) != 1)
			return 1;
		if (sscanf(argv[3], "%hu", &arg[1]) != 1)
			return 1;
		ret = ipc_send_text(argv[1], white, black, arg[0], arg[1]);
		if (ret) {
			perror("ipc_send_text");
			return ret;
		}
	}
	else
		return ipc_send_text("1234567890qwertyuiopasdfghjklzxcvbnm,./';][\\+_)(*&^%$#@!MNBVCXZLKJHGFDSAPOIUYTREWQ{}|:\"<>?",
				     white, black, 40, 39);

}
