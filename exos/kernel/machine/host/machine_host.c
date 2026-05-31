// Host "virtual board" machine layer.
//
// Implements the __machine_* porting interface (kernel/machine/hal.h) on a
// POSIX host (tested on macOS/arm64) so the whole OS can be compiled with
// stock LLVM/clang and run as an ordinary process, without a microcontroller.
//
// Threads are cooperative green threads built on ucontext (getcontext/
// makecontext/swapcontext). Because every exos thread runs on a single OS
// thread, there is no true parallelism: the SysTick/PendSV pair is emulated
// by a virtual tick pumped from the idle thread and a deferred-switch flag
// that mirrors ARM's "PendSV fires on exception return" semantics.
//
// NOTE: this TU is compiled WITHOUT exos/include on the header path. It uses
// the host's real <ucontext.h>/<stdlib.h>/<string.h> (whose stdint/stdbool
// are correct for LP64) and reaches the kernel only through kernel/*.h.

#define _XOPEN_SOURCE 700				// expose the ucontext routines
#if defined(__APPLE__)
#define _DARWIN_C_SOURCE 1				// also expose MAP_ANON on macOS
#endif

#include <ucontext.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include <kernel/machine/hal.h>
#include <kernel/thread.h>
#include <kernel/syscall.h>
#include <kernel/machine/time_hal.h>		// __kernel_tick (under EXOS_OLD_TICK_API)

// ---------------------------------------------------------------------------
// Linker-symbol stand-ins
//
// On a real target a linker script defines the process-stack and TLS template
// boundaries plus the heap. On the host we synthesise them. The main thread
// runs on the real OS stack, so __machine_stack_* only needs to be a valid,
// non-NULL range for the bookkeeping in __thread_init().
// ---------------------------------------------------------------------------

#define EXOS_HOST_HEAP 4194304			// 4 MiB exos heap, registered below

static unsigned char _main_stack_region[4096];
static unsigned char _tbss_region[64];

void * const __machine_stack_start = &_main_stack_region[0];
void * const __machine_stack_end = &_main_stack_region[sizeof(_main_stack_region)];
void * const __machine_tbss_start = &_tbss_region[0];

// Heap backing store. memory.c references &__heap_start__ / &__heap_end__ as
// boundary symbols; alias them onto this array via the assembler.
unsigned char __exos_host_heap[EXOS_HOST_HEAP];

#define _HSTR(x) #x
#define _HSTR2(x) _HSTR(x)

#if defined(__APPLE__)
#define _SYM(s) "_" s					// Mach-O prepends one underscore
#else
#define _SYM(s) s
#endif

__asm__(
	".globl " _SYM("__heap_start__") "\n"
	".set   " _SYM("__heap_start__") ", " _SYM("__exos_host_heap") "\n"
	".globl " _SYM("__heap_end__") "\n"
	".set   " _SYM("__heap_end__") ", " _SYM("__exos_host_heap") "+" _HSTR2(EXOS_HOST_HEAP) "\n");

// ---------------------------------------------------------------------------
// Cooperative scheduler glue (emulated PendSV)
// ---------------------------------------------------------------------------

static ucontext_t _boot_ctx;			// save area for the initial (main) thread
static volatile int _switch_pending;	// set by __machine_req_switch (PendSV pend)
static int _kernel_depth;				// >0 while inside a kernel/tick section

// Each created thread stores a pointer to its ucontext_t in thread->SP. The
// main thread starts with SP == NULL and lazily adopts _boot_ctx.
static inline ucontext_t *_ctx_of(exos_thread_t *t)
{
	if (t->SP == NULL)
		t->SP = &_boot_ctx;
	return (ucontext_t *)t->SP;
}

// Perform any pending context switch. Only runs at the outermost kernel level,
// so a switch never happens in the middle of a tick/syscall handler — exactly
// like a real PendSV deferred until exception return.
static void _maybe_switch(void)
{
	while (_switch_pending)
	{
		_switch_pending = 0;
		exos_thread_t *prev = __running_thread;
		exos_thread_t *next = __kernel_schedule();
		if (next == prev)
			continue;
		__running_thread = next;
		swapcontext(_ctx_of(prev), _ctx_of(next));
	}
}

void __machine_req_switch(void)
{
	_switch_pending = 1;				// pend; actual swap deferred to _maybe_switch
}

// ---------------------------------------------------------------------------
// Syscall entry (emulated SVC)
//
// On ARM __kernel_do traps via SVC; nested calls made from handler mode run
// the dispatcher directly. We mirror that with a depth counter: the deferred
// switch is only taken when the outermost call unwinds.
// ---------------------------------------------------------------------------

#define KERNEL_DO_MAX_ARGS 8

int __kernel_do(EXOS_SYSTEM_FUNC entry, ...)
{
	unsigned long args[KERNEL_DO_MAX_ARGS];
	va_list ap;
	va_start(ap, entry);
	for (int i = 0; i < KERNEL_DO_MAX_ARGS; i++)
		args[i] = va_arg(ap, unsigned long);
	va_end(ap);

	_kernel_depth++;
	int result = entry(args);
	_kernel_depth--;

	if (_kernel_depth == 0)
		_maybe_switch();
	return result;
}

// ---------------------------------------------------------------------------
// Thread stack / context creation
// ---------------------------------------------------------------------------

typedef struct
{
	exos_thread_func_t entry;
	void *arg;
	void (*ret)(void);
} _thread_start_t;

#define HOST_THREAD_STACK (512 * 1024)	// host coroutines need far more than an MCU

static void _trampoline(unsigned hi, unsigned lo)
{
	uintptr_t p = ((uintptr_t)hi << 32) | (uintptr_t)lo;
	_thread_start_t *st = (_thread_start_t *)p;
	st->entry(st->arg);
	st->ret();							// thread.c _exit(): removes thread, switches away
	abort();							// must not return
}

// The kernel passes &stack_end here and stores the resulting value into
// thread->SP. We ignore the (tiny, MCU-sized) exos stack buffer and back each
// thread with an independent mmap'd stack, storing the ucontext_t at its base.
void __machine_init_thread_stack(void **pstack, unsigned long arg, unsigned long pc, unsigned long lr)
{
	void *region = mmap(NULL, HOST_THREAD_STACK, PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANON, -1, 0);
	if (region == MAP_FAILED)
		abort();

	ucontext_t *uc = (ucontext_t *)region;
	_thread_start_t *st = (_thread_start_t *)((unsigned char *)region + sizeof(ucontext_t));
	st->entry = (exos_thread_func_t)pc;
	st->arg = (void *)arg;
	st->ret = (void (*)(void))lr;

	unsigned char *stack_lo = (unsigned char *)(st + 1);
	stack_lo = (unsigned char *)(((uintptr_t)stack_lo + 15) & ~(uintptr_t)15);

	getcontext(uc);
	uc->uc_stack.ss_sp = stack_lo;
	uc->uc_stack.ss_size = (size_t)(((unsigned char *)region + HOST_THREAD_STACK) - stack_lo);
	uc->uc_link = NULL;

	// Pass the 64-bit descriptor pointer to the trampoline as two ints, the
	// portable makecontext idiom.
	makecontext(uc, (void (*)(void))_trampoline, 2,
		(unsigned)((uintptr_t)st >> 32), (unsigned)((uintptr_t)st & 0xFFFFFFFF));

	*pstack = uc;						// becomes thread->SP
}

// Host threads use native compiler TLS (per OS-thread). Since all green threads
// share one OS thread, __thread variables are effectively process-global here;
// the on-stack TLS template used by the ARM port is not needed. (Documented
// limitation: no per-exos-thread TLS isolation on host.)
void __machine_init_thread_local_storage(void **pstack)
{
	(void)pstack;
}

// ---------------------------------------------------------------------------
// Trivial / no-op machine primitives
//
// Cooperative single-OS-thread execution means there is no real concurrency,
// so locks need no atomics and barriers are no-ops.
// ---------------------------------------------------------------------------

int __machine_trylock(unsigned char *lock, unsigned char value)
{
	if (*lock == value)
		return 0;						// already taken
	*lock = value;
	return 1;
}

void __machine_unlock(unsigned char *lock)
{
	*lock = 0;
}

void __machine_dsb(void)
{
	__asm__ volatile ("" ::: "memory");
}

// Console write to the real host stdout. exos defines its own write()/printf()
// in the portable libc, so those names resolve to exos symbols, not libc. We
// reach the kernel's real file descriptor 1 through the raw syscall instead.
long __host_console_write(const void *buf, unsigned long len)
{
	return syscall(SYS_write, 1, buf, len);
}

void __machine_idle(void *args)
{
	(void)args;
	// Pump the virtual tick. The tick is treated like an interrupt: kernel
	// depth is raised so nested syscalls don't switch mid-handler, then any
	// woken thread is switched to once the handler unwinds.
	while (1)
	{
		_kernel_depth++;
		__kernel_tick();
		_kernel_depth--;
		_maybe_switch();
	}
}

// On a real target this programs the SysTick period; the host instead pumps a
// virtual tick from the idle loop (see __machine_idle), so there is no hardware
// timer to configure.
void hal_time_initialize(int period_us)
{
	(void)period_us;
}

void __machine_init(void)
{
	__board_init();
}

void __machine_reset(void)
{
	_Exit(0);
}

void __machine_reboot(void **vbase)
{
	(void)vbase;
	_Exit(0);
}

void __machine_halt(void)
{
	_Exit(0);
}
