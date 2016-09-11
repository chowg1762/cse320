SRCDIR = src
INCDIR = include
BINDIR = bin
BLDDIR = build
RSRCDIR = rsrc
CFLAGS = -Wall -Werror -I$(INCDIR)

BIN = $(BINDIR)/map_reduce
SRC =\
    $(SRCDIR)/main.c \
    $(SRCDIR)/mapreduce.c \

OBJS = $(SRC:$(SRCDIR)/%.c=$(BLDDIR)/%.o)

all: $(BIN)

$(BIN): $(OBJS)
        gcc $(CFLAGS) -o $@ $(OBJS)

$(BLDDIR)/%.o: $(SRCDIR)/%.c
        gcc $(CFLAGS) -c $(SRCDIR)/%.c -o $@

.PHONY: clean
clean:
        rm -f $(OBJS) $(BIN)