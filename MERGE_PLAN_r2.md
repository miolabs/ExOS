# ExOS — r2 → main Merge Plan

Status: planning. Recovery tag `v1` created on `main` (`50bb3b9`), local only.

## 1. Branch topology

- `main` (`50bb3b9`) is a direct ancestor of `r2`.
- `r2` is **195 commits ahead, 0 behind** `main`.
- A merge of `r2` into `main` is therefore a **fast-forward** — no conflicts. The risk is *functional*, not merge mechanics.
- Scale of change: 803 files, +34,139 / −9,436.

## 2. What r2 actually is

Three things bundled in one branch:

1. **Tree reorg** — `kernel/`, `support/`, `include/`, `modules/` moved under a new `exos/` namespace (~514 renames; ~0 files truly deleted).
2. **IDE migration** — Rowley CrossWorks (`.hzp`) → SEGGER Embedded Studio (`.emProject`).
3. **New features** — STM32F4/F7 support, RNG/`random_hal`, USB host+device rewrite with DMA, Ethernet (STM32F7), CDC-NCM networking, iAP2/Apple CP, DAC, and a core IO/event/dispatch/timer kernel overhaul.

Net effect: r2 is an **STM32F4/F7-first rewrite**. Active build targets dropped from ~28 board projects to:

- `test/posix_tests_stm32f4.emProject` (STM32F405RG)
- `test/posix_tests_lpc177x.emProject` (LPC1778)
- `projects/bootloader/bootloader_stm32f4.emProject` (STM32F405RG)
- `exos/support/dm36x/ubl/ubl.hzp`, `exos/support/dm36x/tests/leopard_test.hzp` (surviving CrossWorks)

All 28 old board projects were shelved into `backup/` (still CrossWorks `.hzp`, still referencing the dead root layout).

## 3. Root-cause of breakage — kernel API overhaul

Core types renamed and signatures changed across every driver/board:

```c
// IO driver vtable — renamed types + changed signatures
- int (*Read)(EXOS_IO_ENTRY *io, void *buffer, unsigned long length);
+ int (*Read)(io_entry_t *io, unsigned char *buffer, unsigned length);
+ io_error_t (*Open)(io_entry_t *io, const char *path, io_flags_t flags);  // new

// Event API — added a parameter, removed several functions
- void exos_event_create(EXOS_EVENT *event);
+ void exos_event_create(event_t *event, exos_event_flags_t flags);   // +1 arg
- exos_event_check / exos_event_wait_multiple / exos_event_wait_signals / __set_event  // REMOVED

// Timer — lost EXOS_SIGNAL, now carries an event_t
- int exos_timer_create(EXOS_TIMER*, ..., EXOS_SIGNAL signal);
+ void exos_timer_create(exos_timer_t*, unsigned time, unsigned period);
```

Type renames: `EXOS_IO_ENTRY → io_entry_t`, `EXOS_EVENT → event_t`, `EXOS_MUTEX → mutex_t`,
`EXOS_DISPATCHER → dispatcher_t`, `EXOS_THREAD → exos_thread_t`, `EXOS_TIMER → exos_timer_t`.

A compat shim exists: kernel headers `#define EXOS_EVENT event_t` etc. behind `#ifdef EXOS_OLD`
(`event.h:20`, `io.h:80`, `mutex.h:13`, `dispatch.h:56`, `list.h:65`, `iobuffer.h`). It covers the
**type names only** — not changed signatures or removed functions.

## 4. Per-board regression audit

Commits per platform since `v1` baseline reveal what was migrated vs moved-untouched:

| Platform | Commits since v1 | State |
|---|---|---|
| `stm32f4` | 63 | New, fully on new kernel |
| `stm32f7` | new | New |
| `lpc17` / `lpc177x_8x` | 7 | **Migrated** — backs active `posix_tests_lpc177x`; HAL uses new API |
| `lpc11` | 1 (move only) | **Broken** — untouched against new kernel |
| `lpc2k` | 1 (move only) | **Broken** — incl. one old-style `exos_event_create` |
| `nuc100` | 1 (move only) | **Broken** |
| `nrf51` | 1 (move only) | **Broken** |
| `dm36x` | 1 (move only) | **Broken** (only `ubl`/`leopard_test` survive as active) |

Verdict per backup project:

| Backup project | Target | Verdict |
|---|---|---|
| `exos_lpc17`, `exos_experiment_lpc17` | LPC1768 | Likely fixable (HAL migrated) |
| `exos_net_lpc1778`, `exos_net2_lpc1778`, `exos_canopen_177x_8x`, `exos_iap_lpc177x_8x`, `exos_experiment_lpc177x_8x` | LPC1778 | HAL OK; apps unvalidated (net needs port) |
| `exos_lpc11`, `exos_lcd_lpc11`, `exos_ble_lpc11*`, `exos_snd_lpc11`, `canrfgate_lpc11c24` | LPC11xx | Broken |
| `exos_lpc24`, `exos_experiment_lpc2k`, `printserver_lpc24`, `rfid_lpc24` | LPC2468 | Broken |
| `exos_nuc100` | NUC140 | Broken |
| `exos_nrf51` | nRF51822 | Broken |
| `exos_dm36x` | DM36x | Broken |
| `printserver2_lpc177x`, `umfi_lpc177x`, `printserver_lpc177x` | LPC1778 | Broken (apps on unvalidated path) |

All also carry a secondary break: `.hzp` files reference the old root layout (`$(ProjectDir)/include`,
`$(ProjectDir)/kernel/driver`) and are CrossWorks, not SEGGER `.emProject`.

Nothing was deleted — every HAL still exists under `exos/support/` and `exos/kernel/machine/`. The
loss is functional: ~10 working board families down to 2 (STM32F4/F7) + partial LPC1778.

## 5. Merge strategy

1. Tag the new tip as `v2` once validated; keep `v1` as the old-board support branch.
2. Establish the build baseline — confirm the 3 active `.emProject` targets build clean in SEGGER ES.
3. Decide per old board: **resurrect** (port project + API) or **retire** (document as dropped).
   Resurrect at least one LPC target to prove cross-arch portability still holds.
4. Fast-forward `main → r2` only after steps 2–3, so `main` always points at something buildable.
5. `git push origin v1` to make the recovery point durable for the team.

## 6. Reference fix-list — resurrect LPC1778

### Phase 0 — Already migrated (verified, do not touch)
- `exos/support/lpc17/*` (32 files) — new `event_t` API.
- `exos/kernel/machine/lpc177x_8x/*`, `exos/kernel/machine/armv7m/*` — no old-API usage.
- `test/board/board_mioboard_lpc1778.c` — clean.

### Phase 1 — Non-net LPC1778 app (low effort, mechanical)
1. Don't fix the `.hzp` — replace it. Copy the working template:
   `cp test/posix_tests_lpc177x.emProject projects/<app>_lpc1778.emProject`.
2. Template already encodes the correct recipe:
   - `macros="TargetDir=$(PackagesDir)/LPC1700/;Target=LPC1778;ExosDir=../exos"` (fix relative `ExosDir`)
   - `c_preprocessor_definitions="EXOS_OLD_TICK_API;EXOS_NO_NET;__TARGET_PROCESSOR=$(DeviceName)"`
   - include dirs: `$(ExosDir);$(ExosDir)/include;$(ExosDir)/kernel`
   - linker: `$(ExosDir)/support/lpc17/flash_placement_17xx.xml`, `FLASH_NAME=FLASH1;RAM_NAME=RAM1;RAM2_NAME=RAM2`
3. Source folders (copy from template): `kernel`, `kernel/driver/comm`, `kernel/machine/armv7m`,
   `kernel/machine/lpc177x_8x`, `kernel/posix`, `support/lpc17`, `support/services`,
   `$(TargetDir)/Device/Source`, plus `board` + app files.
4. Add app `main.c`/board config; set the board macro (template uses `BOARD_MIOBOARD`).
5. `EXOS_OLD` decision: add it to defs as a quick bridge for old type names, or rename to `_t` types.

### Phase 2 — Ethernet variant (`exos_net_lpc1778`) — real code work
`exos/kernel/machine/lpc177x_8x/net_lpc17.c:16` fills the old `NET_DRIVER`; kernel now expects
`net_driver_t` (`exos/kernel/driver/net/adapter.h:46`). Vtable mapping:

| Old (`net_lpc17.c`) | New (`net_driver_t`) | Change |
|---|---|---|
| `int Initialize(NET_ADAPTER*)` | `bool Initialize(net_adapter_t*, unsigned phy_unit, const phy_handler_t*)` | +2 params, bool |
| — | `void Start(net_adapter_t*, dispatcher_context_t*)` | NEW, required |
| `LinkUp/LinkDown(NET_ADAPTER*)` | `LinkUp/LinkDown(net_adapter_t*)` | rename only |
| `void* GetInputBuffer(NET_ADAPTER*, unsigned long*)` | `net_buffer_t* GetInputBuffer(net_adapter_t*)` | new return, no len out |
| `void DiscardInputBuffer(...)` | `bool FreeBuffer(net_adapter_t*, net_buffer_t*)` | renamed + unified |
| `void* GetOutputBuffer(NET_ADAPTER*, unsigned long)` | (removed) | drop |
| `int SendOutputBuffer(NET_ADAPTER*, NET_MBUF*, NET_CALLBACK, void*)` | `bool SendOutputBuffer(net_adapter_t*, net_buffer_t*)` | callback model gone |
| — | `void Flush(net_adapter_t*)` | NEW |

Reference: port against `exos/support/stm32f7/eth.c` (already implements `net_driver_t`), swapping
STM32 EMAC access for the existing LPC17 `emac.c`. Then drop `EXOS_NO_NET` and add the
`kernel/driver/net` + `support/services` net sources. ~1 file, ~150 lines.

### Phase 3 — Build & residuals
1. Build in SEGGER ES; fix include/macro errors first.
2. Remaining errors = old type names → add `EXOS_OLD` or rename.
3. Watch the timer `signal`→`event` change (`timer.h`): app timer code needs updating.

Effort: non-net LPC1778 ≈ 1–2 h (project file). Net variant adds the `net_lpc17.c` port.
LPC11/LPC2k/NUC100/nRF51/DM36x are harder — machine code untouched, needs full API migration.
