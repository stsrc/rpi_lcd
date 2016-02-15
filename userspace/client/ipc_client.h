#ifndef _IPC_CLIENT_H_
#define _IPC_CLIENT_H_
#include "ipc.h"
 
int ipc_send_bitmap(uint16_t x, uint16_t y, uint16_t dx, uint16_t dy,
		    uint8_t *mem);
int ipc_send_text(char *text, enum colors font, enum colors background,
		  uint16_t x, uint16_t y);
int ipc_send_rectangle(uint16_t x, uint16_t y, uint16_t dx, uint16_t dy,
		       enum colors color);
int ipc_read_touchscreen(uint16_t *x, uint16_t *y, uint16_t *z);

#endif
