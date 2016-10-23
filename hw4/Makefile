CC := gcc
SRCD := src
BLDD := build
BIND := bin
INCD := include

_SRCF := $(shell find $(SRCD) -type f -name *.c)
_OBJF := $(patsubst $(SRCD)/%,$(BLDD)/%,$(_SRCF:.c=.o))
INC := -I $(INCD)

EXEC := sfish

CFLAGS := -Wall -Werror
DFLAGS := -g -DDEBUG
LIBS := readline

.PHONY: clean all

debug: CFLAGS += -g -DDEBUG
debug: all

all: setup $(EXEC)

setup:
	mkdir -p bin build
	stty intr undef
	stty quit undef
	stty kill undef
	stty eof undef
	stty susp undef

$(EXEC): $(_OBJF)
	$(CC) $^ -o $(BIND)/$@ -l $(LIBS)

$(BLDD)/%.o: $(SRCD)/%.c
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<


reset:
	stty intr ^c
	stty quit ^\\
	stty kill ^u
	stty eof ^d
	stty susp ^z

stat:
	stty -a

clean: reset
	$(RM) -r $(BLDD) $(BIND)
