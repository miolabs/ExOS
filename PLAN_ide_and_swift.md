# ExOS: IDE matrix (Xcode / Visual Studio / VS Code) and a Swift application layer

Status: proposal, 2026-11-03. Branch of record: `feat/host-virtual-board`.

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

Not in the tree: an ARM build outside SEGGER Embedded Studio, any Windows host
layer, any Swift.

Toolchain facts measured today (they drive the Swift design):

| Toolchain | Embedded Swift for `armv7em-none-none-eabi` | Embedded Swift for macOS host |
|---|---|---|
| Xcode 27 (Swift 6.4) | no stdlib shipped | no stdlib shipped |
| swift.org 6.3.3 (swiftly) | yes (also armv6m, armv7m, riscv32, aarch64 bare-metal) | yes (`arm64-apple-macos`) |

A test file compiled for Cortex-M4 at `-Osize` produced a 4.7 KB object with only
`__aeabi_memclr`, `__stack_chk_guard/__stack_chk_fail` and `malloc`-class
symbols undefined. So: **the Swift layer is built by the swift.org toolchain,
invoked from CMake as a plain compiler step. Xcode and Visual Studio never need
to "understand" Swift**; they build C and link objects.

## 1. Goals and non-goals

Goals

1. One source tree, one build description (CMake), three IDEs: Xcode, Visual
   Studio, VS Code. Both the host virtual board and the ARM firmware build from it.
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
│ machine/{armv7m,stm32f4,...} │ machine/host/{posix,win32}     │
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

### Phase 1 — IDE matrix for the C host build (3–5 days)
- **Xcode:** verify `-G Xcode` end to end (schemes for examples, breakpoints in
  kernel code, console output). Add `cmake/presets`: `CMakePresets.json` with
  `host-xcode`, `host-ninja`, `host-vs`, later `stm32f4`.
- **VS Code:** ship `templates/exos-app/.vscode/` (settings, launch, tasks) — the
  README already promises it.
- **Visual Studio (Windows):**
  - New `machine/host/win32/` using Win32 fibers (`ConvertThreadToFiber`,
    `CreateFiber`, `SwitchToFiber`) behind the same `__machine_*` interface;
    move the current ucontext code to `machine/host/posix/`.
  - Compile with the **ClangCL toolset** (`-T ClangCL`), not MSVC: ExOS relies on
    `__weak`, `__attribute__((section))`, `__asm__` labels and GNU statement
    forms that MSVC does not have. ClangCL keeps those on COFF.
  - Heap/stack/TLS boundary symbols: replace the `__asm__` label trick with a
    per-platform C definition (`host_symbols_{posix,win32}.c`).
  - Known MSVC-side friction to budget for: `EXOS_INITIALIZER` section ordering
    on COFF (`.CRT$XCU`-style naming), `-fno-builtin` libc override vs the UCRT
    headers (the two-tier include split already isolates this).
- CI: GitHub Actions matrix macOS (clang, Xcode gen) / Linux (gcc) / Windows
  (VS 2022 + ClangCL), each runs the examples and `test/posix` on the virtual
  board.

### Phase 2 — ARM firmware through CMake (3–4 days)
Prerequisite for Swift on hardware; also what lets Xcode/VS build firmware.
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
- In Xcode/VS the firmware is an external-build target; debugging on hardware
  stays in SEGGER/J-Link, which is fine.

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
- **Windows is the expensive IDE.** Visual Studio needs a new machine layer and
  COFF-specific section handling; VS Code and Xcode are nearly free. If time is
  short, ship Phase 1 without Windows and keep VS Code as the cross-platform path.
- **Hard-float mismatch** could force softfp on the whole firmware; decide in the
  spike before Phase 4 starts.
- **Two build systems for ARM** (SEGGER + CMake) until SEGGER is retired; the
  `.emProject` source list is the source of truth, CMake is transcribed from it
  and the spike's firmware must pass the same `test/posix` suite.

## 5. Order and dependencies

```
P0 ─► P1 (Xcode/VS Code) ─► P1 (Windows, optional)
  └─► P2 (ARM via CMake) ─► P3 (Swift spike) ─► P4 (Exos module) ─► P5
```
P1 and P2 are independent and can run in parallel. P3 needs P2 for the hardware
half only; its host half can start right after P0.
