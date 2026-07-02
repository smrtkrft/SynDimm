# Host testleri — pure (IDF'siz) modüllerin birim testleri.
# Kullanım:  make -C test/host run
# Her test_*.c bağımsız bir binary'dir; `run` hepsini sırayla koşar.

CC      ?= cc
CFLAGS  := -std=c11 -Wall -Wextra -Werror -g

COMP    := ../../components

TESTS   := test_quiet test_gesture test_template test_binding test_offline test_sequence

all: $(TESTS)

test_quiet: test_quiet.c $(COMP)/sd_prefs/src/sd_quiet.c
	$(CC) $(CFLAGS) -I$(COMP)/sd_prefs/include -o $@ $^

test_gesture: test_gesture.c $(COMP)/sd_encoder/src/sd_gesture.c
	$(CC) $(CFLAGS) -I$(COMP)/sd_encoder/include -o $@ $^

test_template: test_template.c $(COMP)/sd_proto/src/sd_template.c
	$(CC) $(CFLAGS) -I$(COMP)/sd_proto/include -o $@ $^

# cJSON vendor kopyası -Wextra ile uyarı üretir — test dosyamız değil,
# o yüzden vendor derlemesinde uyarılar hataya çevrilmez.
# sd_binding, sd_util'in PURE kısmına dayanır (sd_util_str — ESP başlığı yok).
test_binding: test_binding.c $(COMP)/sd_mode_engine/src/sd_binding.c \
              $(COMP)/sd_util/src/sd_util_str.c vendor/cJSON.c
	$(CC) $(CFLAGS) -Wno-error -I$(COMP)/sd_mode_engine/include -I$(COMP)/sd_util/include -Ivendor -o $@ $^

test_offline: test_offline.c $(COMP)/sd_mode_engine/src/sd_offline.c
	$(CC) $(CFLAGS) -I$(COMP)/sd_mode_engine/include -o $@ $^

test_sequence: test_sequence.c $(COMP)/sd_behaviors/src/sd_sequence.c
	$(CC) $(CFLAGS) -I$(COMP)/sd_behaviors/include -o $@ $^

run: all
	@set -e; for t in $(TESTS); do ./$$t; done; \
	echo "ALL HOST TESTS PASSED"

clean:
	rm -f $(TESTS)

.PHONY: all run clean
