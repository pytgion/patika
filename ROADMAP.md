# Patika: Roadmap
## Authoritative Server-Side Pathfinding and Agent Coordination Library

---

## What This Is

A C library for running many moving agents on a grid, server-side. You tick it, it moves agents, it emits events. No engine dependency, no runtime, stable C ABI.

Target users: game backends (MMOs, strategy, tower defense, RTS), headless simulations, any server that needs to move things around a map without shipping a full engine.

---

## Current State (What Actually Exists)

| Area | Status |
|---|---|
| Build | Compiles, `agent_grid` is present — needs verification on clean build |
| Core tick loop | Works: commands → tick → events → snapshot |
| Agent pool | Generational IDs, pool allocator — correct |
| Command queue (MPSC) | **Race condition** — head advanced before slot written |
| Pathfinding | **Greedy hill-climb, hex only** — fails at concave obstacles, no rectangular support |
| Rectangular grid | Config supports it, pathfinding ignores it — only `HEX_DIRS` in `compute_next_step` |
| `compute_patrol` | Implemented but **never called from tick loop** |
| `CMD_SET_BEHAVIOR` | In enum, hits `default: WARN` |
| `CMD_REMOVE_BARRACK` | Stub, explicit WARN logged |
| Guard tile commands (7–10, 13–16) | Stub, all hit `default: WARN` |
| Building commands (17–18) | Stub, all hit `default: WARN` |
| FLEE / GUARD behaviors | Silent fallback to IDLE |
| `ADD_AGENT` payload | Heap-allocated, freed inside `process_command` — fragile |
| Double-buffered snapshots | Correct |
| SPSC event queue | Correct |
| PCG32 RNG | Correct |
| Map load (`patika_load_map`) | Works |

---

## What Stays (Do Not Touch)

- **Generational agent IDs** — correct for tracking agent lifecycle
- **Command/event queue split** — right model for a tick-based server
- **Double-buffered snapshots** — clean reader/writer separation
- **`PatikaConfig` + opaque handle** — good FFI boundary
- **PCG32 seeded RNG** — deterministic tie-breaking
- **Rectangular grid** — keep, make it primary

---

## Phase 1 — Fix What's Broken

### 1.1 Fix the MPSC Race

`mpsc_push` does `atomic_fetch_add` on head, then writes to the slot. Consumer can pop a slot before it's written.

Fix: write slot data first, then flip a per-slot `ready` flag. Consumer checks `ready` before returning the entry. This is a one-file change in `patika_mpsc.c`.

### 1.2 Inline All Command Payloads

`CMD_ADD_AGENT` and `CMD_ADD_BARRACK` malloc a payload that `process_command` frees. This is fragile — if the queue fills up after push but before pop, the pointer is in limbo. It also makes the API awkward for callers.

Fix: move all fields into the inline command union. The union is 32 bytes; `AddAgentPayload` fits. Remove `large_command.payload` entirely.

### 1.3 Wire Patrol into the Tick Loop

`compute_patrol` exists and works. The tick loop in `patika_core.c` never calls it.

Fix: in the tick loop, after handling `STATE_CALCULATING` and `STATE_MOVING`, add a branch for `BEHAVIOR_PATROL` agents in `STATE_IDLE` — call `compute_patrol`, set `STATE_MOVING`.

### 1.4 Verify Clean Build

Confirm full build at `-Wall -Wextra` with zero warnings before moving on. Fix anything that surfaces.

**Deliverable:** Correct command queue, no heap payloads, patrol works, clean build.

---

## Phase 2 — Real Pathfinding

### 2.1 A\* for Rectangular Grid

Replace greedy hill-climb with A\* for `MAP_TYPE_RECTANGULAR`. This is the primary grid type for game backends (tile maps, grid-based games).

The greedy approach fails at any concave obstacle — a wall with a corner, a room entrance, anything non-trivial. A\* handles all of these correctly.

**Node pool:** heap-allocated per call on the server is fine. Size it from config (`max_path_length`). If budget exhausted before goal → `EVENT_STUCK` with the agent's current position. Honest failure.

**Open set:** binary min-heap on f-cost.  
**Closed set:** flat bitfield over grid cells.  
**Heuristic:** Manhattan distance (4-directional) or Chebyshev (8-directional) — pick based on whether diagonal movement is supported.

### 2.2 Keep Hex Grid Working

Hex pathfinding should also become A\* using the existing `get_dist()` as the heuristic. The six-direction table stays.

### 2.3 Connect Reservation Table to Pathfinding

`compute_next_step` ignores `agent_grid` when picking the next tile. A\* node expansion must treat reserved tiles as temporarily blocked. This is what makes multi-agent movement not clip through each other.

**Deliverable:** Agents navigate correctly around any obstacle on rectangular maps. `EVENT_STUCK` only fires on genuinely unreachable goals.

---

## Phase 3 — API Cleanup

### 3.1 Cut or Implement Every Stub Command

A command that silently does nothing is a bug from the caller's perspective. For each stub: implement it or remove it from the enum.

**Implement:**
- `CMD_SET_BEHAVIOR` — needed for runtime behavior changes
- `CMD_REMOVE_BARRACK` — add inline field to command union, implement free

**Remove from enum:**
- `CMD_AGENT_ADD_GUARD_TILE` / `CMD_AGENT_REMOVE_GUARD_TILE` / `CMD_AGENT_CLEAR_GUARD_TILES`
- `CMD_BARRACK_ADD_GUARD_TILE` / `CMD_BARRACK_REMOVE_GUARD_TILE` / `CMD_BARRACK_CLEAR_GUARD_TILES`
- `CMD_ADD_BUILDING` / `CMD_REMOVE_BUILDING`

If guard tiles become needed later, add them back with a real implementation.

### 3.2 Remove FLEE and GUARD Behaviors

Both silently fall back to IDLE. Remove from `AgentBehavior` enum. If either is needed in the future, it gets implemented, not stubbed.

### 3.3 Remove Game-Specific Collision Fields

`PatikaCollisionData.aggression_mask` — game concept, not relevant to movement coordination. Remove.

`INTERACT_ATTACK`, `BUILDING_TOWER`, `BUILDING_TRAP`, `BUILDING_IMMUNITY` — remove from enums. A pathfinding library does not need these.

`faction` / `side` on agent and barrack — keep but rename to `group` / `team`. Neutral naming, still useful for separating agent sets.

### 3.4 Clean Up Source Comments

Remove the inline profanity from `patika_internal.h:185` and `patika_core.c`. Fine in private notes, not in a library anyone else might read.

### 3.5 Add Version Header

```c
// include/patika/version.h
#define PATIKA_VERSION_MAJOR 0
#define PATIKA_VERSION_MINOR 2
#define PATIKA_VERSION_PATCH 0
```

**Deliverable:** Public API makes sense to someone who didn't write it. Zero silent stubs. No dead enum values.

---

## Phase 4 — Server Hardening

### 4.1 Thread Safety Audit

The library is designed for multi-threaded use (MPSC queue, atomic snapshot index). Verify the actual guarantees:

- Multiple threads can call `patika_submit_command` concurrently — confirm MPSC is correct after Phase 1 fix
- Only one thread calls `patika_tick` — document this constraint explicitly
- `patika_get_snapshot` / `patika_poll_events` safe to call from any thread — confirm

Document what is and isn't safe. Callers need to know.

### 4.2 Multiple Concurrent Contexts

Each `PatikaHandle` is independent — confirm there's no global state. If there is any, remove it.

### 4.3 Benchmark Suite

For a representative config (200 agents, 256×256 map, 30% obstacle density):

- Measure `patika_tick()` average and worst-case wall time
- Measure at 50, 100, 200 agents
- Run 10,000 ticks, report ticks/sec

This number belongs in the README. Integrators need to know if this fits in their server frame budget.

### 4.4 Stress Test

- All agents moving to random targets simultaneously
- Dynamic obstacle changes mid-run (`CMD_SET_TILE_STATE` at high rate)
- Verify zero crashes, zero event queue overflows, zero stuck agents on reachable goals

**Deliverable:** Documented thread model, measured performance, no crashes under load.

---

## Milestone Summary

| Phase | Deliverable | Complexity |
|---|---|---|
| 1 | Correct queues, inline payloads, patrol works | Low — targeted fixes |
| 2 | A\* on rectangular grid, reservation-aware | Medium — algorithmic |
| 3 | Clean public API, no stubs or dead code | Low — mechanical |
| 4 | Thread model documented, benchmarked, stress tested | Low–Medium |

---

## Explicit Non-Goals

- Embedded / bare-metal support — not the target
- Flow fields — add only if A\* is measured as a bottleneck at real agent counts
- Network protocol / serialization — caller's problem
- Navmesh — grid is the abstraction, it stays
- Pathfinding-as-a-service / HTTP API — wrong layer
