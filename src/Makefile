# ======================================================================
# Minimal Makefile for the thal-only build of primer3 (C++).
# ======================================================================

CXX      = g++
CC_OPTS  = -g -Wall
O_OPTS   = -O2
CXXFLAGS = $(CC_OPTS) $(O_OPTS) -std=c++17
LDFLAGS  = -g
LDLIBS   = -lm

TEST_EXE = test_thal

all: $(TEST_EXE)

$(TEST_EXE): test_main.o thal.o
	$(CXX) $(LDFLAGS) -o $@ test_main.o thal.o $(LDLIBS)

# We use '-ffloat-store' to prevent undesirable floating-point precision
# differences across platforms.
thal.o: thal.cpp thal.h thal_default_params.h
	$(CXX) -c $(CXXFLAGS) -ffloat-store -o $@ thal.cpp

test_main.o: test_main.cpp thal.h
	$(CXX) -c $(CXXFLAGS) -o $@ test_main.cpp

clean:
	-rm -f *.o $(TEST_EXE) *~ core
