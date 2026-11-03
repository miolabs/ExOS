# Starter ExOS application (host "virtual board").
#
# ExOS is consumed as a source dependency: add it as a git submodule under
# exos/ (see README) and add_subdirectory() it. That defines the exos_add_app()
# helper plus the exos::host / exos::virtual_board targets.
#
#   git submodule add <exos-repo-url> exos
#   cmake -S . -B build && cmake --build build && ./build/my_app

cmake_minimum_required(VERSION 3.21)
project(my_exos_app C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Emit compile_commands.json so VS Code's IntelliSense (and clangd) see the real
# flags. Harmless for the Xcode generator (which ignores it).
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Debug by default so Xcode / VS Code get symbols.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

# Pull in ExOS. EXPECTS the submodule at ./exos -- fail early with a clear
# message if it hasn't been checked out yet.
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/exos/CMakeLists.txt")
	message(FATAL_ERROR
		"ExOS submodule missing. Run:\n"
		"    git submodule update --init --recursive")
endif()
add_subdirectory(exos)

# Build the app. Write a normal int main(); exos_add_app wires it up as the
# kernel's initial thread and links it against the default virtual board.
exos_add_app(my_app SOURCES app/main.c)
