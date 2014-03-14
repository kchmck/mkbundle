BINARY = mkbundle

SRC = \
      block.c \
      bundle.c \
      mkbundle.c \
      sdnv.c \
      strbuf.c \

OBJ = $(SRC:.c=.o)

ALL_CFLAGS += -Wall -Wextra -std=c11 -pipe
ALL_CFLAGS += $(CFLAGS)

ALL_LDFLAGS += $(LDFLAGS)

all: $(BINARY)

$(BINARY): $(OBJ)
	$(CC) -o $@ $^ $(ALL_LDFLAGS)

test:
	$(MAKE) CFLAGS="-DMKBUNDLE_TEST $(CFLAGS)" BINARY=test-mkbundle -B

%.o: %.c
	$(CC) -c $(ALL_CFLAGS) $< -o $@

.PHONY: all test
