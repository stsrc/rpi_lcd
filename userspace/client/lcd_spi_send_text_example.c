#include "ipc_client.h"

int main(int argc, char *argv[])
{
	if (argc == 2)
		return ipc_send_text(argv[1], white, black, 40, 39);
	else
		return ipc_send_text("1234567890qwertyuiopasdfghjklzxcvbnm,./';][\\+_)(*&^%$#@!MNBVCXZLKJHGFDSAPOIUYTREWQ{}|:\"<>?",
				     white, black, 40, 39);

}
