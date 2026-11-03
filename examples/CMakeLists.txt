# Bundled ExOS host examples. Built only when ExOS is the top-level project.
#
# Each is an ordinary executable that runs the whole OS on the virtual board.
#   ./blink           two threads blinking LEDs (POSIX pthreads)
#   ./hello_service   a background service registered at kernel init

exos_add_app(blink         SOURCES blink/main.c)
exos_add_app(hello_service SOURCES hello_service/main.c hello_service/service.c)
