# Lightweight CLI front-end for the CMake build.
#
# This is just a convenience wrapper so you can `make` / `make run` from a
# terminal without remembering the CMake invocation. CMake remains the source
# of truth (it is also what Xcode and VS Code use) -- see CMakeLists.txt.
#
#   make        configure + build (Debug)
#   make run    build, then run the app
#   make clean  remove the build directory

BUILD   ?= build
APP     ?= my_app
CONFIG  ?= Debug

.PHONY: all build run clean

all: build

$(BUILD)/CMakeCache.txt:
	cmake -S . -B $(BUILD) -DCMAKE_BUILD_TYPE=$(CONFIG)

build: $(BUILD)/CMakeCache.txt
	cmake --build $(BUILD)

run: build
	./$(BUILD)/$(APP)

clean:
	rm -rf $(BUILD)
