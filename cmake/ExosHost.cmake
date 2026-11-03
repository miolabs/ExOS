# ExosHost.cmake -- build ExOS as a host "virtual board" library.
#
# Including this file (it is pulled in by the repo-root CMakeLists.txt) defines:
#
#   exos::host            STATIC lib: the kernel + POSIX layer + host machine
#                         glue. Link this to run ExOS as an ordinary process.
#   exos::virtual_board   STATIC lib: the default virtual board (console on
#                         stdout, no-op LEDs). Optional -- supply your own board
#                         instead by not linking it.
#   exos_add_app(<name> <sources...>)
#                         Convenience: builds an executable whose main() becomes
#                         the kernel's initial thread and links it against the
#                         virtual board. Write a normal int main(); it just runs.
#
# Two-tier include strategy (the crux of the host port):
#   * Portable kernel/POSIX TUs compile WITH exos's freestanding libc
#     (exos/include) so bool == int and they use exos's own string/stdio/etc.
#   * The host machine glue compiles WITHOUT exos/include, against the system
#     headers, so it gets a real ucontext and LP64-correct stdint, reaching the
#     kernel only through exos/kernel/*.h.
# Both tiers agree on the shared struct ABI (thread_t, io_entry_t, ...), which
# use only plain int/long/pointer types.

# Repo root that contains the exos/ source tree.
get_filename_component(EXOS_SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(EXOS_K "${EXOS_SDK_ROOT}/exos")

# Flags shared by every ExOS TU and re-exported to apps (see INTERFACE below).
#  -fms-extensions : exos uses anonymous struct members (node_t;).
#  -fno-builtin    : exos ships its own libc; clang must not assume builtin
#                    semantics for malloc/memcpy/printf/... (matches ARM build).
set(EXOS_HOST_CFLAGS
	-fms-extensions
	-fno-builtin
	-Wno-deprecated-declarations
	-Wno-macro-redefined
	-Wno-microsoft-anon-tag
	-Wno-incompatible-library-redeclaration
	-Wno-pointer-to-int-cast
	-Wno-int-to-pointer-cast
	-Wno-pointer-sign)

# Defines for the in-library build (not needed by app code):
#  STDOUT_STREAM    : the device the POSIX stdio layer opens for stdout.
#  EXOS_OLD_TICK_API: selects the __kernel_tick entry the host idle loop pumps.
#  EXOS_NO_NET      : the host port builds without the network stack.
#  EXOS_APP_MAIN    : startup.c calls this instead of main() (the process owns
#                     the real main; the app's main is renamed to match -- see
#                     exos_add_app below).
set(EXOS_HOST_DEFS
	STDOUT_STREAM="/dev/console"
	EXOS_OLD_TICK_API
	EXOS_NO_NET
	EXOS_APP_MAIN=exos_app_main)

# --- portable kernel + POSIX (tier A) -------------------------------------

set(EXOS_PORTABLE_SRCS
	${EXOS_K}/kernel/startup.c
	${EXOS_K}/kernel/thread.c
	${EXOS_K}/kernel/thread_pool.c
	${EXOS_K}/kernel/event.c
	${EXOS_K}/kernel/mutex.c
	${EXOS_K}/kernel/list.c
	${EXOS_K}/kernel/timer.c
	${EXOS_K}/kernel/memory.c
	${EXOS_K}/kernel/io.c
	${EXOS_K}/kernel/tree.c
	${EXOS_K}/kernel/port.c
	${EXOS_K}/kernel/panic.c
	${EXOS_K}/kernel/types.c
	${EXOS_K}/kernel/verbose.c
	${EXOS_K}/kernel/dispatch.c
	${EXOS_K}/kernel/fifo.c
	${EXOS_K}/kernel/iobuffer.c
	${EXOS_K}/kernel/datetime.c
	${EXOS_K}/kernel/machine/hal.c
	${EXOS_K}/support/services/init.c
	${EXOS_K}/kernel/posix/posix.c
	${EXOS_K}/kernel/posix/pthread.c
	${EXOS_K}/kernel/posix/pthread_cond.c
	${EXOS_K}/kernel/posix/pthread_time.c
	${EXOS_K}/kernel/posix/stdio.c
	${EXOS_K}/kernel/posix/stdlib.c
	${EXOS_K}/kernel/posix/string.c
	${EXOS_K}/kernel/posix/unistd.c
	${EXOS_K}/kernel/posix/errno.c
	${EXOS_K}/kernel/posix/fcntl.c)

add_library(exos_host_portable OBJECT ${EXOS_PORTABLE_SRCS})
target_include_directories(exos_host_portable PRIVATE
	${EXOS_K}/include ${EXOS_K} ${EXOS_K}/support)
target_compile_definitions(exos_host_portable PRIVATE ${EXOS_HOST_DEFS})
target_compile_options(exos_host_portable PRIVATE ${EXOS_HOST_CFLAGS})

# --- host machine glue (tier B: system headers) ---------------------------

set(EXOS_MACHINE_SRCS
	${EXOS_K}/kernel/machine/host/machine_host.c
	${EXOS_K}/kernel/machine/host/host_start.c)

add_library(exos_host_machine OBJECT ${EXOS_MACHINE_SRCS})
target_include_directories(exos_host_machine PRIVATE ${EXOS_K})   # kernel headers only
target_compile_definitions(exos_host_machine PRIVATE EXOS_OLD_TICK_API)
target_compile_options(exos_host_machine PRIVATE ${EXOS_HOST_CFLAGS})

# --- default virtual board (tier A) ---------------------------------------

add_library(exos_virtual_board_obj OBJECT ${EXOS_K}/bsp/host_virtual/board_virtual.c)
target_include_directories(exos_virtual_board_obj PRIVATE
	${EXOS_K}/include ${EXOS_K} ${EXOS_K}/support ${EXOS_K}/bsp/host_virtual)
target_compile_definitions(exos_virtual_board_obj PRIVATE EXOS_OLD_TICK_API EXOS_NO_NET)
target_compile_options(exos_virtual_board_obj PRIVATE ${EXOS_HOST_CFLAGS})

# --- consumer-facing aggregate targets ------------------------------------
#
# ExOS is delivered to the application as a set of OBJECT libraries whose object
# files are spliced *directly* into the executable, rather than as a STATIC
# archive. This is deliberate and load-bearing:
#
#  * Drivers/services register themselves passively via EXOS_INITIALIZER, which
#    only deposits a pointer in the __exos_ctors section. Nothing references
#    their object, so an archive link would cull them and they would silently
#    never run. Object-library objects are always included.
#  * startup.c carries __weak no-op defaults (e.g. __services_init) next to the
#    reference site. With an archive, the linker satisfies the reference from
#    that in-object weak def and never pulls the strong override; with direct
#    objects the strong definition wins -- exactly as in a plain object link.
#
# Linking object libraries (even transitively through these INTERFACE targets)
# adds their objects to the final executable, so no -force_load / --whole-archive
# is needed. That also keeps the Xcode and Visual Studio generators working,
# which do not reliably build a STATIC lib assembled purely from $<TARGET_OBJECTS>.

# exos::host -- the OS core (kernel + POSIX + host machine glue). The object
# files are injected into the consumer as INTERFACE sources via $<TARGET_OBJECTS>
# (the reliable way to splice object-library output into a linking target across
# generators and directory boundaries); the include dirs and flags ride along as
# the usual INTERFACE usage requirements.
add_library(exos_host INTERFACE)
add_library(exos::host ALIAS exos_host)
target_sources(exos_host INTERFACE
	$<TARGET_OBJECTS:exos_host_portable>
	$<TARGET_OBJECTS:exos_host_machine>)
# What an application needs to compile against ExOS: exos's libc + kernel
# headers, and the same dialect flags.
target_include_directories(exos_host INTERFACE
	${EXOS_K}/include ${EXOS_K} ${EXOS_K}/support)
target_compile_options(exos_host INTERFACE ${EXOS_HOST_CFLAGS})

# exos::virtual_board -- the default board (console on stdout, no-op LEDs) on
# top of exos::host. Link this to get a runnable OS; supply your own board by
# linking exos::host plus your own board object instead. Its object rides along
# as an INTERFACE source, and it pulls in exos::host transitively.
add_library(exos_virtual_board INTERFACE)
add_library(exos::virtual_board ALIAS exos_virtual_board)
target_sources(exos_virtual_board INTERFACE $<TARGET_OBJECTS:exos_virtual_board_obj>)
target_link_libraries(exos_virtual_board INTERFACE exos_host)
target_include_directories(exos_virtual_board INTERFACE ${EXOS_K}/bsp/host_virtual)

# --- app helper -----------------------------------------------------------

# exos_add_app(<name> [SOURCES] <sources...>)
# Builds an executable that runs on the virtual board. The app's main() is
# renamed to exos_app_main so the kernel (which owns the process main) can call
# it as the initial thread -- so just write a normal main().
function(exos_add_app TARGET)
	cmake_parse_arguments(APP "" "" "SOURCES" ${ARGN})
	if(NOT APP_SOURCES)
		set(APP_SOURCES ${APP_UNPARSED_ARGUMENTS})
	endif()
	add_executable(${TARGET} ${APP_SOURCES})
	target_compile_definitions(${TARGET} PRIVATE main=exos_app_main)
	target_link_libraries(${TARGET} PRIVATE exos::virtual_board)
endfunction()
