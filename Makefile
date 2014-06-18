# See copyright notice in Copying.

BINARY = mkbundle

SRC = \
      block.c \
      mkbundle.c \
      parser.c \
      primary-block.c \
      sdnv.c \
      strbuf.c \
      ui.c \
      util.c \

OBJ = $(SRC:.c=.o)

ALL_CFLAGS += -Wall -Wextra -Werror -std=c11 -pipe
ALL_CFLAGS += -Ijsmn
ALL_CFLAGS += $(CFLAGS)

ALL_LDFLAGS += -Ljsmn -ljsmn
ALL_LDFLAGS += $(LDFLAGS)

all: $(BINARY)

$(BINARY): $(OBJ)
	$(MAKE) -C jsmn
	$(CC) -o $@ $^ $(ALL_LDFLAGS)

test:
	$(MAKE) CFLAGS="-DMKBUNDLE_TEST -O0 -g $(CFLAGS)" BINARY=test-mkbundle -B

%.o: %.c
	$(CC) -c $(ALL_CFLAGS) $< -o $@

.PHONY: all test
