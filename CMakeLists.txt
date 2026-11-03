# ExOS -- host "virtual board" build.
#
# This file makes ExOS consumable as a dependency: an application repository
# adds ExOS as a git submodule and does
#
#     add_subdirectory(path/to/exos)
#     exos_add_app(my_app SOURCES src/main.c)
#
# to compile and run the whole OS as an ordinary host process (no hardware).
# See cmake/ExosHost.cmake for the targets it defines (exos::host,
# exos::virtual_board, exos_add_app). The ARM firmware build is unaffected --
# that lives in the SEGGER Embedded Studio .emProject files, not here.

# 3.21+: ExOS is delivered as OBJECT libraries spliced into the executable;
# transitive object propagation through INTERFACE targets is reliable from here.
cmake_minimum_required(VERSION 3.21)
project(exos C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Default to a debuggable build so IDEs (Xcode / VS Code) get symbols.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmake/ExosHost.cmake)

# Build the bundled examples only when ExOS is the top-level project, so a
# consuming application that add_subdirectory()s us doesn't inherit them.
if(PROJECT_IS_TOP_LEVEL)
	add_subdirectory(examples)
endif()
