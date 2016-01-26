#include "ipc_client.h"

int main(int argc, char *argv[])
{
	int ret;
	if (argc == 2) {
		ret = ipc_send_text(argv[1], white, black, 40, 39);
		if (ret) {
			perror("ipc_send_text");
			return ret;
		}
	}
	else
		return ipc_send_text("1234567890qwertyuiopasdfghjklzxcvbnm,./';][\\+_)(*&^%$#@!MNBVCXZLKJHGFDSAPOIUYTREWQ{}|:\"<>?",
				     white, black, 40, 39);

}
