CC = gcc

CFLAGS := -O0 -Wall -std=gnu99 -ggdb -lm
SRCFILES := ipc_server.c lcd_spi_daemon.c 
OBJFILES := $(patsubst %.c, %.o, $(SRCFILES)) 
PROGFILES := lcd_spi_daemon 

.PHONY: all clean

all: $(PROGFILES)
$(PROGFILES) : ipc_server.o

clean:
	rm -f $(OBJFILES) $(PROGFILES) *~
