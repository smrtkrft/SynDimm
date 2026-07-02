# Host testleri — pure (IDF'siz) modüllerin birim testleri.
# Kullanım:  make -C test/host run
# Her test_*.c bağımsız bir binary'dir; `run` hepsini sırayla koşar.

CC      ?= cc
CFLAGS  := -std=c11 -Wall -Wextra -Werror -g

COMP    := ../../components

TESTS   := test_quiet

all: $(TESTS)

test_quiet: test_quiet.c $(COMP)/sd_buzzer/src/sd_quiet.c
	$(CC) $(CFLAGS) -I$(COMP)/sd_buzzer/include -o $@ $^

run: all
	@set -e; for t in $(TESTS); do ./$$t; done; \
	echo "ALL HOST TESTS PASSED"

clean:
	rm -f $(TESTS)

.PHONY: all run clean
