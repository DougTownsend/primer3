# ======================================================================
# Minimal Makefile for the thal-only build of primer3.
# ======================================================================

CC       = gcc
CC_OPTS  = -g -Wall
O_OPTS   = -O2
CFLAGS   = $(CC_OPTS) $(O_OPTS)
LDFLAGS  = -g
LDLIBS   = -lm

TEST_EXE = test_thal

all: $(TEST_EXE)

$(TEST_EXE): test_main.o thal.o
	$(CC) $(LDFLAGS) -o $@ test_main.o thal.o $(LDLIBS)

# We use '-ffloat-store' to prevent undesirable floating-point precision
# differences across platforms.
thal.o: thal.c thal.h thal_default_params.h
	$(CC) -c $(CFLAGS) -ffloat-store -o $@ thal.c

test_main.o: test_main.c thal.h
	$(CC) -c $(CFLAGS) -o $@ test_main.c

clean:
	-rm -f *.o $(TEST_EXE) *~ core
