# ExOS: IDE support (Xcode / VS Code) and a Swift application layer

Status: proposal, 2026-11-03 (rescoped same day: Visual Studio on Windows is out of
scope for now). Branch of record: `feat/host-virtual-board`, PR #1.

## 0. Where we are

Already in the tree (uncommitted on `feat/host-virtual-board`):

- `exos/kernel/machine/host/` — ucontext green-thread port, host entry point.
- `exos/bsp/host_virtual/` — dummy board (LEDs no-op, console → stdout).
- `CMakeLists.txt` + `cmake/ExosHost.cmake` — `exos::host`, `exos::virtual_board`,
  `exos_add_app()`; ExOS is consumed as a submodule by an app repo.
- `examples/blink`, `examples/hello_service`, `templates/exos-app` (README already
  documents `-G Xcode` and VS Code CMake Tools).
- Two fixes pending in the diff: heap boundary symbols as real labels (needed for
  a static-lib link) and `pthread_join(NULL)`.

Not in the tree: an ARM build outside SEGGER Embedded Studio, any Swift.

Toolchain facts measured today (they drive the Swift design):

| Toolchain | Embedded Swift for `armv7em-none-none-eabi` | Embedded Swift for macOS host |
|---|---|---|
| Xcode 27 (Swift 6.4) | no stdlib shipped | no stdlib shipped |
| swift.org 6.3.3 (swiftly) | yes (also armv6m, armv7m, riscv32, aarch64 bare-metal) | yes (`arm64-apple-macos`) |

A test file compiled for Cortex-M4 at `-Osize` produced a 4.7 KB object with only
`__aeabi_memclr`, `__stack_chk_guard/__stack_chk_fail` and `malloc`-class
symbols undefined. So: **the Swift layer is built by the swift.org toolchain,
invoked from CMake as a plain compiler step. Xcode and VS Code never need to
"understand" Swift**; they build C and link objects.

## 1. Goals and non-goals

Goals

1. One source tree, one build description (CMake), two IDEs: Xcode and VS Code
   (macOS/Linux). Both the host virtual board and the ARM firmware build from it.
2. Application layer optionally in Swift (Embedded Swift), same `.swift` sources
   for the host virtual board and the microcontroller.
3. Core stays C. Swift shares the *application-facing* API, not the kernel.
4. Fit constrained parts: Cortex-M4 (STM32F4/F7, LPC1778) first class; Cortex-M0
   (LPC11, nRF51) feasible in `-no-allocations` mode.

Non-goals

- Rewriting kernel, drivers or POSIX layer in Swift.
- Swift Concurrency, Foundation, existentials, reflection: all excluded by
  Embedded Swift anyway.
- Replacing the SEGGER `.emProject` files: they keep working; CMake is added
  beside them.
- Visual Studio on Windows. Not now; see the note at the end of Phase 1.

## 2. Architecture

```
┌──────────────────────────────────────────────────────────────┐
│ Application  (C, or Swift via module `Exos`)                   │
├──────────────────────────────────────────────────────────────┤
│ exos/swift/Exos        Swift wrappers (Thread, Mutex, Event,  │
│                        Timer, IO, Console, Board, Dispatch)   │
│ exos/swift/ExosC       clang module: exos_swift.h shim        │  ← Swift only
├──────────────────────────────────────────────────────────────┤
│ kernel/posix  + modules + services        (C, unchanged)      │
├──────────────────────────────────────────────────────────────┤
│ machine/{armv7m,stm32f4,...} │ machine/host (ucontext)        │
└──────────────────────────────────────────────────────────────┘
```

Design rules for the Swift layer

- **Shim, not raw headers.** `exos/swift/ExosC/exos_swift.h` is a small,
  Swift-clean C header (plain structs, opaque pointers, no `-fms-extensions`
  anonymous members, no function-like macros). Swift imports only this. The
  kernel headers stay free to evolve; the shim is the ABI contract.
- **Two flavours, like the C API.** Static flavour: `~Copyable` structs that own
  the `exos_*_t` storage inline (no heap, usable with `-no-allocations`).
  Dynamic flavour: final classes over the POSIX calls (needs `malloc`, which
  ExOS already provides).
- **Entry point.** Swift app exports `@_cdecl("exos_app_main")`; the C side
  already supports `-DEXOS_APP_MAIN=exos_app_main` (startup.c). Same symbol on
  host and target.
- **Runtime services Embedded Swift needs**, all supplied by ExOS's own libc:
  `malloc/free/posix_memalign` (add `posix_memalign` to `kernel/posix/stdlib.c`),
  `memcpy/memset/memmove`, `putchar` (for `print`, routes to `/dev/console`),
  `__aeabi_*` builtins from GCC-ARM's `libgcc` on target / compiler-rt on host.
  Stack protector: build with `-Xfrontend -disable-stack-protector` or provide
  the two symbols.
- **Same flags everywhere:** `-enable-experimental-feature Embedded -wmo
  -parse-as-library -Osize` plus `-target` and `-Xcc` include paths. Host adds
  nothing else; target adds `-Xcc -mcpu/-mfloat-abi` matching the C flags.

## 3. Phases

### Phase 0 — land the host port (½ day)
Commit the pending host work (CMake, examples, template, the two fixes), open the
PR for `feat/host-virtual-board`. Everything below branches from it.

### Phase 1 — Xcode and VS Code for the C host build (1–2 days)
- **Presets:** `CMakePresets.json` with `host-ninja`, `host-xcode` (and later
  `stm32f4`), so both IDEs and the terminal configure the same way.
- **Xcode:** verify `-G Xcode` end to end (schemes for the examples, breakpoints
  in kernel code, console output in the Xcode pane). Fix whatever the generator
  trips on (object-library splice, `-fms-extensions` in the generated flags).
- **VS Code:** ship `templates/exos-app/.vscode/` (settings for CMake Tools,
  launch for lldb/gdb, tasks) — the README already promises it. IntelliSense
  from `compile_commands.json`.
- **CI:** GitHub Actions on macOS (clang, Ninja and Xcode generators) and Linux
  (gcc) building the examples and `test/posix` on the virtual board.

Visual Studio on Windows, for later: it would need a `machine/host/win32`
layer on Win32 fibers, the ClangCL toolset (ExOS uses `__weak`, section
attributes and `__asm__` labels that MSVC lacks), per-platform heap/stack
boundary symbols instead of the `__asm__` labels, and COFF section ordering for
`EXOS_INITIALIZER`. Nothing in Phases 1–5 blocks adding it afterwards.

### Phase 2 — ARM firmware through CMake (3–4 days)
Prerequisite for Swift on hardware; also what lets Xcode/VS Code build firmware.
- `cmake/toolchains/arm-none-eabi.cmake` (GCC-ARM, `-mcpu=cortex-m4
  -mfpu=fpv4-sp-d16`, float ABI decided in the spike below, `-fms-extensions
  -fno-builtin`, no standard libs).
- `cmake/ExosTarget.cmake`: source lists transcribed from
  `test/posix_tests_stm32f4.emProject` (59 TUs) into `exos::kernel`,
  `exos::posix`, `exos::machine_stm32f4`, `exos::bsp_<board>`; same
  object-library splice as the host build so `EXOS_INITIALIZER` sections survive.
- Linker script, startup and vectors from `machine/stm32f4`; `exos_add_firmware()`
  producing `.elf/.hex` and a `flash` target (J-Link / OpenOCD).
- Acceptance: `test/posix` firmware built by CMake runs the same tests on the
  STM32F4 board as the SEGGER build. LPC1778 second.
- In Xcode/VS Code the firmware is an external-build target; debugging on
  hardware stays in SEGGER/J-Link, which is fine.

### Phase 3 — Swift spike (2–3 days, decides go/no-go)
Minimal `blink` in Swift, same source, two builds:
1. Host: `swiftc -enable-experimental-feature Embedded` → object → linked into
   `exos_add_app`. Runs in Xcode on the virtual board.
2. STM32F4: object for `armv7em-none-none-eabi`, linked by the Phase 2 firmware
   target.
Questions the spike must answer, with numbers:
- **Float ABI.** The swift.org stdlib is shipped for `armv7em-none-none-eabi`
  only (no `eabihf`). Either compile C with `-mfloat-abi=softfp` or verify that
  `-Xcc -mfloat-abi=hard` on the Swift side links cleanly against hard-float C.
  Pick one and encode it in the toolchain file.
- **Code size:** flash/RAM delta of the Swift blink vs the C blink, with and
  without `print`. Target: < 16 KB flash for the base layer.
- **Importer:** does ClangImporter accept the existing kernel headers with
  `-Xcc -fms-extensions`, or do we go shim-only from day one (plan assumes shim).
- Missing symbols list → what to add to ExOS libc (`posix_memalign`, etc.).

### Phase 4 — the `Exos` Swift module (1–2 weeks)
`exos/swift/` with `ExosC` (modulemap + shim) and `Exos`:
- `Thread` (static + dynamic), `Mutex`, `Event`, `Timer`, `sleep(ms:)`.
- `IO` (open/read/write/ioctl over `io_entry_t`), `Console` (`print` already
  works through `putchar`; add formatted write without Foundation).
- `Board` (LEDs/outputs from `bsp/*/board.h`), `Dispatch` (dispatcher wrapper).
- `exos_add_swift_app(<name> SWIFT_SOURCES ... C_SOURCES ...)` in CMake: finds
  the swift.org toolchain (`EXOS_SWIFT_TOOLCHAIN` cache var, default the swiftly
  install), runs `swiftc` as a custom command, links the object. Works for host
  and firmware targets alike.
- Tests: the Swift examples run on the virtual board in CI (host), and a
  hardware checklist for STM32F4.

### Phase 5 — small-MCU validation, docs, templates (3–4 days)
- Build the Swift blink for `armv6m-none-none-eabi` with `-no-allocations`
  (nRF51/LPC11 class); record flash/RAM.
- `templates/exos-app-swift`, README updates, `examples/blink_swift`.
- Document the toolchain requirement: swift.org toolchain via swiftly; Xcode's
  Swift is not used for ExOS.

## 4. Risks

- **Toolchain drift.** Embedded Swift is still "experimental feature"; pin the
  swift.org version in `CMakePresets.json` and CI.
- **Hard-float mismatch** could force softfp on the whole firmware; decide in the
  spike before Phase 4 starts.
- **Two build systems for ARM** (SEGGER + CMake) until SEGGER is retired; the
  `.emProject` source list is the source of truth, CMake is transcribed from it
  and the spike's firmware must pass the same `test/posix` suite.

## 5. Order and dependencies

```
P0 ─► P1 (Xcode/VS Code)
  └─► P2 (ARM via CMake) ─► P3 (Swift spike) ─► P4 (Exos module) ─► P5
```
P1 and P2 are independent and can run in parallel. P3 needs P2 for the hardware
half only; its host half can start right after P0.
