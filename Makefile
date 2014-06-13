BINARY = mkbundle

SRC = \
      block.c \
      primary-block.c \
      mkbundle.c \
      sdnv.c \
      strbuf.c \

OBJ = $(SRC:.c=.o)

ALL_CFLAGS += -Wall -Wextra -Werror -std=c11 -pipe
ALL_CFLAGS += -Wstrict-prototypes -Wshadow -Wpointer-arith -Wcast-qual \
          -Wmissing-prototypes -Wconversion -Wformat=2 \
          -Wstrict-overflow=5 -pedantic -Wno-missing-braces \
          -Winline -Wlogical-op -Wstrict-aliasing \
          -Wredundant-decls -Wwrite-strings \
          -Wmissing-include-dirs -Wuninitialized
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
