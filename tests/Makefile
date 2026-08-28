# Host-side unit tests (GCC/Clang). No devkitARM required.
CXX      ?= g++
CXXFLAGS ?= -std=c++20 -Wall -Wextra -Werror -O2
INCLUDES := -Ihost_bn -I../include

.PHONY: all test clean

all: test

test_physics: test_physics.cpp ../src/physics.cpp ../src/bridge_system.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ test_physics.cpp ../src/physics.cpp ../src/bridge_system.cpp

test: test_physics
	./test_physics

clean:
	rm -f test_physics test_physics.exe
