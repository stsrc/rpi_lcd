CC = gcc

CFLAGS := -O0 -Wall -std=gnu99 -ggdb
SRCFILES := $(wildcard *.c)
OBJFILES := $(patsubst %.c, %.o, $(SRCFILES)) events.o
PROGFILES := $(patsubst %.c, %, $(SRCFILES))

.PHONY: all clean

all: $(PROGFILES)
$(PROGFILES) : 

clean:
	rm -f $(OBJFILES) $(PROGFILES) *~
