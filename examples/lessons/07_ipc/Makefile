CC	= gcc
CFLAGS	= -g -O0 -Wall -Wextra -std=c99 -D_POSIX_C_SOURCE=200809L

# Signal examples (Section 4)
SIG_TARGETS = sig_basic sig_sigaction sig_safe_flag sig_graceful sig_watchdog

# Shared memory examples (existing)
SHM_TARGETS = reader writer

ALL_TARGETS = $(SIG_TARGETS) $(SHM_TARGETS)

.PHONY: all sig shm clean

all: $(ALL_TARGETS)

sig: $(SIG_TARGETS)

shm: $(SHM_TARGETS)

sig_basic: sig_basic.c
	$(CC) $(CFLAGS) -o $@ $<

sig_sigaction: sig_sigaction.c
	$(CC) $(CFLAGS) -o $@ $<

sig_safe_flag: sig_safe_flag.c
	$(CC) $(CFLAGS) -o $@ $<

sig_graceful: sig_graceful_shutdown.c
	$(CC) $(CFLAGS) -o $@ $<

sig_watchdog: sig_watchdog.c
	$(CC) $(CFLAGS) -o $@ $<

reader: reader.c
	$(CC) $(CFLAGS) -o $@ $< -lrt -lpthread

writer: writer.c
	$(CC) $(CFLAGS) -o $@ $< -lrt -lpthread

clean:
	rm -f $(ALL_TARGETS)