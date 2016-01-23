#ifndef _IPC_H_
#define _IPC_H_

#include <stdint.h>

#define WRITE_TEXT	1
#define WRITE_BITMAP	2

struct ipc_buffer {
	int cmd;
	uint8_t *mem;
	uint16_t x;
	uint16_t y;
	uint16_t dx;
	uint16_t dy;
};

/*
 * ALL PROBLEMS WITH ALLIGNMENT - GET INFORMATIONS!
 */

#include "lcd_spi.h"

int ipc_main(int fd);

#endif
