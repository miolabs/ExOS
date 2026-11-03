# Host virtual-board tests. Each test is an ordinary ExOS application whose
# main() returns when it is done; the kernel then resets, which on the host
# exits the process with status 0. CTest treats a timeout or a non-zero exit as
# a failure.
#
#   cmake --preset host-ninja && cmake --build --preset host-ninja
#   ctest --preset host-ninja

exos_add_app(posix_tests SOURCES posix/basic_pthread_tests.c)
add_test(NAME posix_tests COMMAND posix_tests)
set_tests_properties(posix_tests PROPERTIES TIMEOUT 60)
