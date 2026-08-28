# Host-side unit tests (GCC/Clang). No devkitARM required.
CXX      ?= g++
CXXFLAGS ?= -std=c++20 -Wall -Wextra -Werror -O2
INCLUDES := -Ihost_bn -I../include

.PHONY: all test clean

all: test

test_physics: test_physics.cpp ../src/physics.cpp ../src/bridge_system.cpp ../src/engine.cpp ../src/level.cpp ../src/run_score.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ test_physics.cpp ../src/physics.cpp ../src/bridge_system.cpp ../src/engine.cpp ../src/level.cpp ../src/run_score.cpp

test_progress: test_progress.cpp ../src/progress.cpp ../src/engine.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ test_progress.cpp ../src/progress.cpp ../src/engine.cpp

test_save_data: test_save_data.cpp ../src/save_data.cpp ../src/progress.cpp ../src/engine.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ test_save_data.cpp ../src/save_data.cpp ../src/progress.cpp ../src/engine.cpp

test: test_physics test_progress test_save_data
	./test_physics
	./test_progress
	./test_save_data
	python3 ../tools/gen_audio.py --check

clean:
	rm -f test_physics test_progress test_save_data test_physics.exe test_progress.exe test_save_data.exe
