CC = gcc

CFLAGS := -O3 -Wall -std=gnu99
SRCFILES := $(wildcard lcd_spi*.c)
OBJFILES := $(patsubst %.c, %.o, $(SRCFILES)) ipc_client.o
PROGFILES := $(patsubst %.c, %, $(SRCFILES))

.PHONY: all clean

all: $(PROGFILES)
$(PROGFILES) : ipc_client.o

clean:
	rm -f $(OBJFILES) $(PROGFILES) *~
