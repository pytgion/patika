# Patika: Roadmap

*Verified against the working tree, 2026-09-04 — every row checked against the source
or produced by running the binaries.*

---

## What This Is

A C library for running many moving agents on a grid, server-side. You tick it, it
moves agents, it emits events. Links as a standalone library with a stable C ABI.

Target users: game backends (MMOs, strategy, tower defense, RTS) and headless
simulations.

---

## Current State

| Area | Status |
|---|---|
| Build | ✅ Warning-free at `-Wall -Wextra -Wpedantic` |
| Core tick loop | ✅ commands → tick → events → snapshot |
| Agent pool | ✅ Generational IDs, pool allocator |
| Command queue (MPSC) | ✅ Slot written first, published via per-slot `ready` flag (release/acquire) |
| Command payloads | ✅ Inline in the command union; no heap allocation crosses the queue |
| Pathfinding | ✅ A\*, both grid types, concave obstacles handled |
| Rectangular grid | ✅ 4-directional, Manhattan heuristic |
| Hexagonal grid | ✅ 6-direction table, hex-distance heuristic |
| Reservation-aware routing | ✅ Expansion treats tiles reserved this tick as blocked |
| Path caching | ✅ Path walked across ticks, discarded on goal change |
| Bounded search | ✅ `max_path_nodes` budget; exhaustion emits `EVENT_STUCK` |
| `compute_patrol` | ✅ Called from the tick loop (`patika_core.c:162`) |
| `CMD_SET_BEHAVIOR` | ✅ Implemented |
| `CMD_REMOVE_BARRACK` | ✅ Implemented |
| Dead enum values | ✅ Guard-tile, building, FLEE and GUARD entries removed |
| Double-buffered snapshots | ✅ |
| SPSC event queue | ✅ |
| PCG32 RNG | ✅ Seeded, deterministic |
| Map load (`patika_load_map`) | ✅ |
| Thread-safety contract | ✅ Documented per function in `include/patika/api.h` |
| Multiple concurrent handles | ✅ No shared mutable state except the log mutex |
| Benchmark suite | ✅ 50/100/200 agents, numbers in the README |
| Version header | ✅ `0.3.0` |
| Test suites | ✅ 10 suites, 85 assertions; 4 files still disabled (§3) |
| `tools/` visualizer | ❌ Stale and unreferenced by the build (§4) |
| Flow fields / sectors | ➖ Cut from the public API until implemented; internal scaffolding kept |
| Tick worst case | ⚠️ 42–208 ms under mass replanning (§2) |

Phases 1–4 of the previous roadmap are complete. What follows is what is left.

---

## What Stays (Do Not Touch)

- **Generational agent IDs** — correct for tracking agent lifecycle across an FFI boundary
- **Command/event queue split** — right model for a tick-based server
- **Double-buffered snapshots** — clean reader/writer separation
- **`PatikaConfig` + opaque handle** — good FFI boundary
- **PCG32 seeded RNG** — deterministic tie-breaking
- **Rectangular grid as primary** — hex stays supported

---

## 1. Flow fields and sectors

The public API used to declare four functions with no definition anywhere in `src/`
(`patika_compute_flow_field`, `patika_free_flow_field`, `patika_rebuild_flow_field`,
`patika_rebuild_sectors`), plus `CMD_SET_FLOW_FIELD`, which reached `process_command`'s
`default:` branch. Calling any of them was a link error. They are now cut from
`api.h`, `enums.h` and the command union.

Still present and still declaration-only, internal to the library:
`flow_field_compute`, `flow_field_free`, `flow_field_get_dir`, `flow_field_step` in
`src/internal/patika_internal.h`, along with `PatikaFlowField`, the `flow_fields[]`
slots and `SectorGrid` in the context. `PatikaConfig.sector_size` is accepted and
ignored. These are private, so they cost callers nothing; implement or remove them
when §2 reaches step 3.

---

## 2. Pathfinding tail latency

200 agents on 256×256: 1.09 ms average, 207 ms worst (full table in the README). Every
agent runs its own A\* against the same obstacle field and the cost is not spread
across ticks.

Cheapest first:

1. **Budget per tick rather than per agent.** Cap total node expansions across all
   agents; unserved agents stay in `STATE_CALCULATING` and are served next tick.
2. **Share paths between agents with the same goal.**
3. **Flow fields.** The intended answer for many-agents-one-goal, worth building after
   1 and 2 are measured.

Measure after each step; the benchmark exists, keep the README numbers current.

---

## 3. Remaining disabled tests

`test_agent_pool`, `test_barrack_pool`, `test_map`, `test_mpsc_queue` and `test_rng`
are re-enabled — the suite is now 10 files and 85 assertions. Four files remain
commented out in `CMakeLists.txt`:

| Suite | State |
|---|---|
| `test_spsc_queue` | 2 failures, stale expectations: tests expect the old `-1` sentinel, `spsc_push`/`spsc_pop` now return `PatikaError` (`PATIKA_ERR_CAPACITY == 4`) |
| `test_commands` | 1 failure: `test_cmd_set_goal` expects `STATE_MOVING` one tick after `CMD_SET_GOAL`, gets `STATE_CALCULATING`. Decide whether the extra tick is intended, then fix the test or the handler |
| `test_stress_queues`, `test_stress_capacity` | Don't compile — call `patika_add_agent_sync`, removed. Port to `patika_submit_command` + `patika_tick`, or delete |

---

## 4. Terminal visualizer

`tools/patika_vis.c` and `tools/vis_demo.c` render the live context to ANSI. Neither is
referenced by `CMakeLists.txt`, so nothing builds them, and both have drifted out of
sync with the library:

| File | Breakage |
|---|---|
| `vis_demo.c` | Builds commands through the removed heap-payload API (`cmd.large_command.payload`), and sets `faction` / `side` / `collision_data.aggression_mask`, all renamed or removed |
| `patika_vis.c` | Switches on `BEHAVIOR_GUARD` and `BEHAVIOR_FLEE`, removed from the enum |

Fixing both is mechanical — inline the payloads, rename to `group` / `team`, drop the
two dead cases — and then an optional `PATIKA_BUILD_TOOLS` target makes it buildable
from a clean checkout. A recording of agents routing around obstacles is the one thing
the README cannot say in prose.

---

## Housekeeping

- `CMakeLists.txt` says `project(patika_c VERSION 1.0.0)`; `version.h` says `0.3.0`.
- `src/tests/test_stress_capacity.c:319` has an inline profanity comment.
- The build never exports `compile_commands.json`, so clangd-based editors index
  `src/` and `src/tests/` with no include paths and report phantom errors. One line:
  `set(CMAKE_EXPORT_COMPILE_COMMANDS ON)`.
