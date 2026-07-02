# Host testleri — pure (IDF'siz) modüllerin birim testleri.
# Kullanım:  make -C test/host run
# Her test_*.c bağımsız bir binary'dir; `run` hepsini sırayla koşar.

CC      ?= cc
CFLAGS  := -std=c11 -Wall -Wextra -Werror -g

COMP    := ../../components

TESTS   := test_quiet test_gesture test_template test_binding

all: $(TESTS)

test_quiet: test_quiet.c $(COMP)/sd_buzzer/src/sd_quiet.c
	$(CC) $(CFLAGS) -I$(COMP)/sd_buzzer/include -o $@ $^

test_gesture: test_gesture.c $(COMP)/sd_encoder/src/sd_gesture.c
	$(CC) $(CFLAGS) -I$(COMP)/sd_encoder/include -o $@ $^

test_template: test_template.c $(COMP)/sd_proto/src/sd_template.c
	$(CC) $(CFLAGS) -I$(COMP)/sd_proto/include -o $@ $^

# cJSON vendor kopyası -Wextra ile uyarı üretir — test dosyamız değil,
# o yüzden vendor derlemesinde uyarılar hataya çevrilmez.
test_binding: test_binding.c $(COMP)/sd_mode_engine/src/sd_binding.c vendor/cJSON.c
	$(CC) $(CFLAGS) -Wno-error -I$(COMP)/sd_mode_engine/include -Ivendor -o $@ $^

run: all
	@set -e; for t in $(TESTS); do ./$$t; done; \
	echo "ALL HOST TESTS PASSED"

clean:
	rm -f $(TESTS)

.PHONY: all run clean
