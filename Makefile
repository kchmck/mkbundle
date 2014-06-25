# See copyright notice in Copying.

BINARY = mkbundle

SRC = \
      block.c \
      ext-block.c \
      mkbundle.c \
      parser.c \
      primary-block.c \
      sdnv.c \
      strbuf.c \
      ui.c \
      util.c \

OBJ = $(SRC:.c=.o)

ALL_CFLAGS += -Wall -Wextra -Werror -std=c11 -pipe
ALL_CFLAGS += -Wstrict-prototypes -Wshadow -Wpointer-arith -Wcast-qual \
              -Wconversion -Wformat=2 -Wstrict-overflow=5 -pedantic \
              -Wno-missing-braces -Winline -Wstrict-aliasing \
              -Wredundant-decls -Wwrite-strings -Wmissing-include-dirs \
              -Wuninitialized
ALL_CFLAGS += -Ijsmn
ALL_CFLAGS += $(CFLAGS)

ALL_LDFLAGS += -Ljsmn -ljsmn
ALL_LDFLAGS += $(LDFLAGS)

ifeq ($(DEBUG), 1)
    ALL_CFLAGS += -O0 -g
endif

ifeq ($(OPTIMIZE), 1)
    ALL_CFLAGS += -flto -O2
    ALL_LDFLAGS += -flto -O2
endif

ifeq ($(NATIVE), 1)
    ALL_CFLAGS += -march=native
    ALL_LDFLAGS += -march=native
endif

PREFIX = /usr/local

all: $(BINARY)

$(BINARY): $(OBJ)
	$(MAKE) -C jsmn
	$(CC) -o $@ $^ $(ALL_LDFLAGS)

test:
	$(MAKE) CFLAGS="-DMKBUNDLE_TEST -O0 -g $(CFLAGS)" BINARY=test-mkbundle -B

%.o: %.c
	$(CC) -c $(ALL_CFLAGS) $< -o $@

install: $(BINARY)
	install -D -m 755 $< "$(DESTDIR)$(PREFIX)/bin/mkbundle"

uninstall:
	rm "$(DESTDIR)$(PREFIX)/bin/mkbundle"

clean:
	$(MAKE) -C jsmn clean
	-rm -f $(OBJ)

distclean: clean
	-rm -f $(BINARY)

.PHONY: all test clean distclean install uninstall
