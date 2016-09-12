SRCDIR = src
INCDIR = include
BINDIR = bin
BLDDIR = build
RSRCDIR = rsrc

CFLAGS = -g -Wall -Werror -I$(INCDIR) -I/usr/include

BIN = $(BINDIR)/mapreduce
SRC =\
	$(SRCDIR)/main.c \
	$(SRCDIR)/map_reduce.c \

OBJS = $(SRC:$(SRCDIR)/%.c=$(BLDDIR)/%.o)

all: $(BIN)

$(BIN): $(OBJS)
		gcc $(CFLAGS) -o $@ $(OBJS)

$(BLDDIR)/%.o: $(SRCDIR)/%.c
		gcc $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
		rm -f $(OBJS) $(BIN)