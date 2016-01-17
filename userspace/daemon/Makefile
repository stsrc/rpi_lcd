CC = gcc

CFLAGS := -O0 -Wall -std=gnu99 -ggdb
SRCFILES := ipc.c lcd_spi_daemon.c
OBJFILES := $(patsubst %.c, %.o, $(SRCFILES)) 
PROGFILES := lcd_spi_daemon

.PHONY: all clean

all: $(PROGFILES)
$(PROGFILES) : $(OBJFILES)	

clean:
	rm -f $(OBJFILES) $(PROGFILES) *~
