# Patika

C11 library for moving agents on a grid, server-side. Submit commands from any thread,
tick it, read positions and events. Links as a standalone library with a stable C ABI.

Used by [Germ Storm](https://github.com/pytgion/germ-storm).

**v0.3.0** · C11 · MIT · macOS / Linux / Windows

---

## Concurrency model

```
producer threads ──► MPSC command queue ──┐
                                          ▼
                                    patika_tick()  (exactly one thread)
                                          │
                        ┌─────────────────┴─────────────────┐
                        ▼                                   ▼
             double-buffered snapshot              SPSC event queue
                        │                                   │
                        ▼                                   ▼
              any reader thread                    one consumer thread
```

- **MPSC command queue.** Producers claim a slot with `fetch_add`, write it, then publish
  it with a per-slot `ready` flag (release). The consumer acquires that flag before
  reading. The submit path is lock-free.
- **Double-buffered snapshots.** `patika_tick` writes one buffer; readers get the other
  via an atomic index.
- **Generational agent IDs.** An ID is `{slot, generation}`; reusing a slot bumps the
  generation, so a stale ID fails lookup instead of addressing the new occupant.
- **Allocation happens once, at create.** Agents are pool-allocated; A\* scratch (open
  heap, closed bitfield, node pool) is allocated in `patika_create` and reused each tick.
- **Seeded PCG32.** Same seed, same simulation.

---

## Build

```sh
./build.sh              # Linux
./build_mac.sh          # macOS
build.bat               # Windows
```

Or:

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Produces `libpatika_core.{so,dylib,dll}`. Tests build by default
(`-DPATIKA_BUILD_TESTS=OFF` to skip).

Builds warning-free at `-Wall -Wextra -Wpedantic`.

## Usage

```c
#include <patika/api.h>

PatikaConfig cfg = {
    .grid_type          = MAP_TYPE_RECTANGULAR,
    .max_agents         = 10000,
    .max_barracks       = 100,
    .grid_width         = 512,
    .grid_height        = 512,
    .command_queue_size = 100000,
    .event_queue_size   = 50000,
    .rng_seed           = 1337,
    .max_path_nodes     = 4096,   /* A* expansion budget per agent per tick */
};

PatikaHandle sim = patika_create(&cfg);
patika_load_map(sim, map_states, 512, 512);

/* any thread */
PatikaCommand cmd = {0};
cmd.type = CMD_ADD_AGENT;
cmd.add_agent.start_q = 4;
cmd.add_agent.start_r = 9;
cmd.add_agent.out_agent_id = &id;
patika_submit_command(sim, &cmd);

/* one thread, in your server loop */
patika_tick(sim);

/* any thread */
const PatikaSnapshot *snap = patika_get_snapshot(sim);

/* one consumer thread */
PatikaEvent events[256];
uint32_t n = patika_poll_events(sim, events, 256);

patika_destroy(sim);
```

Commands are a tagged union passed by value, copied into the queue slot.

### Thread safety

| Function | Rule |
|---|---|
| `patika_submit_command(s)` | Any number of threads, concurrently |
| `patika_tick` | Exactly one thread per handle |
| `patika_get_snapshot` | Any thread, any time |
| `patika_poll_events` | Exactly one consumer thread per handle |
| `patika_create` / `destroy` / `load_map` | No other thread may hold the handle |

Handles are independent; multiple simulations can run in one process. The only
process-wide state is the log mutex. Same contract in
[`include/patika/api.h`](include/patika/api.h).

---

## Pathfinding

A\* on both grid types: rectangular (4-directional, Manhattan heuristic) and hexagonal
(6-direction table, hex-distance heuristic).

- Node expansion treats tiles reserved by other agents this tick as blocked, so agents
  route around each other.
- `max_path_nodes` caps expansion per agent per tick. Exhausting it emits `EVENT_STUCK`
  at the agent's current position.
- A computed path is cached and walked over subsequent ticks, discarded when the goal
  changes or the route is invalidated.

Events: `EVENT_REACHED_GOAL`, `EVENT_STUCK`, `EVENT_BLOCKED`, `EVENT_REPLAN_NEEDED`,
`EVENT_AGENT_REMOVED`.

---

## Performance

Apple M1, Release `-O3`, gcc. 256×256 rectangular grid, ~30% obstacle band across the
middle, every agent pathing to the far side. 10,000 ticks.

| Agents | Total | Ticks/s | Avg tick | Worst tick |
|---:|---:|---:|---:|---:|
| 50 | 2.08 s | 4,802 | 0.208 ms | 41.9 ms |
| 100 | 5.10 s | 1,963 | 0.510 ms | 97.9 ms |
| 200 | 10.91 s | 917 | 1.091 ms | 207.7 ms |

Reproduce with `./build/test_benchmark`.

Each agent runs its own A\* and the cost is not spread across ticks, so a tick where
many agents replan at once costs far above the average. Counts above 200 agents are
untested.

---

## Tests

Unity framework, wired into CTest.

```sh
cd build && ctest --output-on-failure
```

10 suites, 85 assertions passing: agent pool, barrack pool, map, MPSC queue, RNG,
pathfinding (hex and rect, concave obstacles, reservations), basic integration,
multi-agent integration, stress (moving obstacles, rapid spawn/destroy), benchmark.

4 files under `src/tests/` remain disabled in `CMakeLists.txt`
([ROADMAP](ROADMAP.md#3-remaining-disabled-tests)).

---

## License

MIT — see [LICENSE](LICENSE).
