# Host unit tests for RunningLinreg
# Usage: make -C tests [CXX=clang++] [STD=c++17] [RNGBUFFER_DIR=path/to/RngBuffer/src]

STD ?= gnu++11
# RngBuffer sources: a sibling checkout named RngBuffer
RNGBUFFER_DIR ?= $(wildcard ../../RngBuffer/src)
ifeq ($(RNGBUFFER_DIR),)
$(error RngBuffer sources not found: set RNGBUFFER_DIR=path/to/RngBuffer/src)
endif

SAN = -fsanitize=address,undefined -fno-sanitize-recover=all
CXXFLAGS = -std=$(STD) -g -O1 -Wall -Wextra -Wpedantic -Wshadow -Werror $(SAN) \
           -I../src -I$(RNGBUFFER_DIR)
# RngBuffer is another library: compiled as C by the same compiler, without -Werror
RBFLAGS = -x c -std=c99 -g -O1 $(SAN) -I$(RNGBUFFER_DIR)

test: test_runninglinreg
	./test_runninglinreg

test_runninglinreg: test_runninglinreg.cpp ../src/RunningLinreg.cpp ../src/RunningLinreg.h RngBuffer.o
	$(CXX) $(CXXFLAGS) $< RngBuffer.o -o $@ -lm

RngBuffer.o: $(RNGBUFFER_DIR)/RngBuffer.c $(RNGBUFFER_DIR)/RngBuffer.h
	$(CXX) $(RBFLAGS) -c $< -o $@

clean:
	rm -f test_runninglinreg RngBuffer.o

.PHONY: test clean
