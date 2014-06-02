CFILES := CircularQueue.c main.c
PROG := sopeProj2
CFLAGS := -Wall -Wextra -Wno-unused-parameter -g
LDFLAGS := -pthread -lm -lrt

CFLAGS += -MMD
CC := gcc

OBJFILES := $(CFILES:.c=.o)
DEPFILES := $(CFILES:.c=.d)

BINDIR := bin

$(BINDIR)/$(PROG) : $(OBJFILES)
		mkdir -p $(BINDIR)
		$(LINK.o) -o $@ $^ $(LDFLAGS)
		rm *.d
		rm *.o
clean :
		rm -rf $(BINDIR) $(OBJFILES) $(DEPFILES)

-include $(DEPFILES)
