# Patika Pathfinding Library - Development Roadmap

**Version:** 1.0.0  
**Target:** Production-ready C library for 15k+ agent real-time pathfinding  
**Timeline:** who knows? (single developer)  
**Last Updated:** 2025-10-28

---

## Progress Overview
```
Phase 1: Foundation        [80%] ████████░░
Phase 2: Core Features     [60%] ██████░░░░
Phase 3: Testing           [30%] ███░░░░░░░
Phase 4: Polish            [ 0%] ░░░░░░░░░░
```

---

## Phase 1: Foundation (Week 1-2) - CURRENT

**Goal:** Stable build system, correct data structures, basic API working.

### 1.1 Build System & Infrastructure [COMPLETE]
- [x] CMakeLists.txt with modular .c files
- [x] Header organization (public/internal separation)
- [x] DLL export/import macros (PATIKA_API)
- [x] Cross-platform build script (build.sh/build.bat)
- [x] Compile with -Wall -Wextra, zero warnings

### 1.2 Core Data Structures [IN PROGRESS]
- [x] Agent pool with generation-based IDs
- [x] Barrack pool basic structure
- [x] Map grid (tile state + occupancy counter)
- [x] Fix agent_pool_free() parameter bug [FIXED]
- [ ] Fix agent_pool_init() off-by-one bug (capacity-2 should be capacity-1)
- [ ] Add occupancy tracking to MapTile
- [ ] Validate free-list integrity (unit test)

### 1.3 Lock-Free Queues [IN PROGRESS]
- [x] MPSC command queue (basic implementation)
- [x] SPSC event queue (basic implementation)
- [ ] Fix CAS loop in mpsc_push() (current version has race condition)
- [ ] Add queue capacity watermark tracking
- [ ] Test queue under concurrent load (4+ threads)

### 1.4 Basic API [IN PROGRESS]
- [x] patika_create/destroy lifecycle
- [x] patika_load_map
- [x] patika_submit_command (single)
- [x] patika_tick() integration
- [x] patika_poll_events()
- [ ] Add error logging callback (patika_set_log_callback)
- [ ] Add file logging utility (patika_log_to_file)

**Milestone 1:** Library compiles, links, and can create/destroy context. [COMPLETE]

---

## Phase 2: Core Simulation (Week 3-4)

**Goal:** 1000 agents moving on map, basic pathfinding working, events emitted.

### 2.1 Command Processing [IN PROGRESS]
- [x] CMD_ADD_AGENT handler
- [x] CMD_SET_GOAL handler
- [ ] CMD_REMOVE_AGENT handler
- [ ] CMD_BIND_BARRACK handler
- [ ] CMD_SET_TILE_STATE handler (dynamic obstacles)
- [ ] CMD_ADD_BARRACK handler
- [ ] Batch command optimization (process multiple per tick)

### 2.2 Pathfinding [IN PROGRESS]
- [x] Greedy hex neighbor selection
- [x] Distance heuristic (Euclidean)
- [ ] Tile occupancy conflict resolution (try_acquire protocol)
- [ ] Stuck detection after N failed moves
- [ ] Path replan trigger on blocked tile
- [ ] Cube-distance heuristic (more accurate for hex)
- [ ] Agent collision avoidance (basic)

### 2.3 Snapshot System [MOSTLY COMPLETE]
- [x] Double-buffered snapshots
- [x] Atomic version increment
- [x] AgentSnapshot population
- [ ] BarrackSnapshot population
- [ ] Validate snapshot consistency (no torn reads)

### 2.4 Event System [IN PROGRESS]
- [x] EVENT_STUCK emission
- [ ] EVENT_REACHED_GOAL emission
- [ ] EVENT_BLOCKED emission
- [ ] EVENT_REPLAN_NEEDED emission
- [ ] EVENT_AGENT_REMOVED emission
- [ ] Event rate limiting (prevent flood)

**Milestone 2:** 1000 agents pathfinding, events flowing, no crashes.

---

## Phase 3: Testing & Validation (Week 5)

**Goal:** Comprehensive test coverage, valgrind-clean, deterministic replay.

### 3.1 Unit Tests [PRIORITY]
- [ ] Agent Pool:
  - [ ] Allocation/deallocation stress test (10k cycles)
  - [ ] Generation counter overflow test
  - [ ] Stale ID rejection test
  - [ ] Free-list corruption detection
  
- [ ] Command Queue:
  - [ ] Single-producer, single-consumer test
  - [ ] Multi-producer test (4 threads x 1000 commands)
  - [ ] Queue full backpressure test
  - [ ] Command ordering verification
  
- [ ] Pathfinding:
  - [ ] Straight-line movement test
  - [ ] Obstacle avoidance test
  - [ ] Stuck agent detection test
  - [ ] Deterministic replay (same seed -> same moves)
  
- [ ] Map/Grid:
  - [ ] Bounds checking (negative coordinates)
  - [ ] Tile state transitions
  - [ ] Occupancy counter saturation
  
- [ ] Snapshot:
  - [ ] Version monotonicity test
  - [ ] No data races under concurrent read
  - [ ] Snapshot size validation

### 3.2 Integration Tests
- [ ] Scenario 1: 5k agents, open field, converge to center
- [ ] Scenario 2: 10k agents, narrow corridor bottleneck
- [ ] Scenario 3: 15k agents, dynamic obstacles (tiles close mid-run)
- [ ] Scenario 4: 100 barracks spawning agents continuously

### 3.3 Performance Tests
- [ ] Tick time measurement (target: <2ms for 15k agents)
- [ ] Memory footprint (should be ~100MB for 16k agents)
- [ ] Queue latency (command submission -> processing time)
- [ ] Event throughput (max events/sec)

### 3.4 Memory Safety
- [ ] Valgrind: Zero memory leaks, zero invalid reads/writes
- [ ] AddressSanitizer: Clean run on all tests
- [ ] ThreadSanitizer: No data races
- [ ] Fuzzing with random command sequences (AFL/libFuzzer)

**Milestone 3:** All tests green, valgrind-clean, 15k agents @ 60 Hz stable.

---

## Phase 4: Optimization & Polish (Week 6-7)

**Goal:** Production-ready performance, API documentation, examples.

### 4.1 Performance Optimization
- [ ] Cache-line align hot structures (AgentSlot)
- [ ] SIMD pathfinding (AVX2 for 4 agents parallel)
- [ ] Sector-based A* (hierarchical pathfinding)
- [ ] Agent state batching (group by state for better cache locality)
- [ ] Lock-free statistics (avoid atomic contention)

### 4.2 API Refinements
- [ ] Add patika_get_agent_by_id() helper
- [ ] Add patika_get_barrack_agents() iterator
- [ ] Add patika_set_log_callback() for debugging
- [ ] Error code string conversion (patika_strerror())
- [ ] API version macros (PATIKA_VERSION_MAJOR/MINOR)

### 4.3 Documentation
- [ ] API Reference: Doxygen comments for all public functions
- [ ] Usage Guide: Step-by-step integration tutorial
- [ ] Architecture Doc: Explain command/event flow, threading model
- [ ] Performance Guide: Tuning tips, profiling tools
- [ ] Migration Guide: From old agent_core.h to new API

### 4.4 Examples
- [ ] example_basic.c: Single-threaded, 100 agents
- [ ] example_threaded.c: Dedicated pathfinding thread
- [ ] example_godot.cpp: GDExtension integration skeleton
- [ ] example_stress.c: 15k agents performance demo

### 4.5 Tooling
- [ ] Visualization tool (dump snapshot to JSON, render with Python/JS)
- [ ] Replay system (record commands, replay deterministically)
- [ ] Profiling script (perf/Instruments wrapper)

**Milestone 4:** Production-ready 1.0 release, published to GitHub.

---

## Phase 5: Advanced Features (Week 8+) - FUTURE

**Optional enhancements, not blocking 1.0 release.**

### 5.1 Advanced Pathfinding
- [ ] Full A* with sector graph
- [ ] Flow field pathfinding (for swarms)
- [ ] Dynamic cost maps (terrain difficulty)
- [ ] Formation movement (group cohesion)
- [ ] Steering behaviors (boids-like flocking)

### 5.2 Multi-Threading
- [ ] Parallel command processing (shard by spatial region)
- [ ] Background A* computation (dedicated thread)
- [ ] Work-stealing agent update scheduler

### 5.3 Simulation Features
- [ ] Agent AI (goal prioritization, behavior trees)
- [ ] Barrack intrusive linked lists (current: O(N) iteration)
- [ ] Save/load simulation state (serialize to file)
- [ ] Hot-reload map (swap terrain without restart)
- [ ] Agent vision/fog of war

### 5.4 Platform Support
- [ ] WebAssembly build (Emscripten)
- [ ] Mobile (Android/iOS)
- [ ] Console (PlayStation/Xbox/Switch)

---

## Critical Path (Next 2 Weeks)

**To reach Milestone 2, complete these in order:**

### Week 1:
1. [x] Fix agent_pool_free() bug [DONE]
2. [ ] Fix agent_pool_init() off-by-one
3. [ ] Add occupancy tracking to MapTile
4. [ ] Implement tile acquire/release protocol
5. [ ] Complete all missing command handlers
6. [ ] Add logging system (patika_log.c)
7. [ ] Write 10 basic unit tests

### Week 2:
8. [ ] Implement stuck detection logic
9. [ ] Add EVENT_REACHED_GOAL emission
10. [ ] Test with 1000 agents (stress test)
11. [ ] Valgrind run (fix any leaks)
12. [ ] Write integration test (5k agents scenario)

---

## Success Metrics

| Metric                        | Target    | Current   | Status      |
|-------------------------------|-----------|-----------|-------------|
| Build Time                    | <10s      | ~3s       | PASS        |
| Unit Test Coverage            | >80%      | ~30%      | IN PROGRESS |
| 15k Agents Tick Time          | <2ms      | Untested  | PENDING     |
| Memory Usage (16k agents)     | <120MB    | Untested  | PENDING     |
| Valgrind Clean                | 0 leaks   | Unknown   | PENDING     |
| API Stability                 | No breaks | 1.0-alpha | IN PROGRESS |

---

## Issue Tracker

### CRITICAL (Blocking Milestone 2)
1. agent_pool_init() off-by-one bug (capacity-2 should be capacity-1)
2. mpsc_push() race condition (CAS loop doesn't properly reserve slot)
3. Missing CMD_REMOVE_AGENT handler
4. No occupancy tracking (agents can stack on same tile)
5. No logging system for debugging

### HIGH PRIORITY
6. Incomplete EVENT_REACHED_GOAL emission
7. BarrackSnapshot not populated in update_snapshot()
8. No unit tests for agent pool
9. No integration tests

### MEDIUM PRIORITY
10. Missing API documentation
11. No example programs
12. Performance not measured

---

## Logging System Requirements

### Functional Requirements:
- Thread-safe logging from multiple threads
- Configurable log levels (DEBUG, INFO, WARN, ERROR)
- File output with rotation (max size/age)
- Optional callback for custom handlers
- Minimal performance overhead (<1% tick time)
- Compile-time disable for release builds

### API Design:
```c
typedef enum {
    PATIKA_LOG_DEBUG,
    PATIKA_LOG_INFO,
    PATIKA_LOG_WARN,
    PATIKA_LOG_ERROR
} PatikaLogLevel;

typedef void (*PatikaLogCallback)(PatikaLogLevel level, const char* message);

// Set log file (NULL to disable file logging)
PATIKA_API void patika_log_set_file(const char* filepath);

// Set log callback (NULL to disable callback)
PATIKA_API void patika_log_set_callback(PatikaLogCallback callback);

// Set minimum log level
PATIKA_API void patika_log_set_level(PatikaLogLevel min_level);

// Internal logging macros (used within library)
#ifdef PATIKA_DEBUG
  #define PATIKA_LOG_DEBUG(fmt, ...) patika_log_internal(PATIKA_LOG_DEBUG, fmt, ##__VA_ARGS__)
  #define PATIKA_LOG_INFO(fmt, ...)  patika_log_internal(PATIKA_LOG_INFO, fmt, ##__VA_ARGS__)
#else
  #define PATIKA_LOG_DEBUG(fmt, ...) ((void)0)
  #define PATIKA_LOG_INFO(fmt, ...)  ((void)0)
#endif
#define PATIKA_LOG_WARN(fmt, ...)  patika_log_internal(PATIKA_LOG_WARN, fmt, ##__VA_ARGS__)
#define PATIKA_LOG_ERROR(fmt, ...) patika_log_internal(PATIKA_LOG_ERROR, fmt, ##__VA_ARGS__)
```

### Implementation Files:
- `src/patika_log.c` - Core logging implementation
- `src/internal/patika_log.h` - Internal logging header
- `include/patika_log.h` - Public logging API (optional, for users)

---

## Test Infrastructure Requirements

### Test Framework:
- Minimal custom framework (no external dependencies)
- Each test is a function returning 0 (pass) or -1 (fail)
- Test runner aggregates results and reports

### Test File Structure:
```
tests/
├── test_agent_pool.c       # Agent allocation, free, generation
├── test_command_queue.c    # MPSC queue operations
├── test_event_queue.c      # SPSC queue operations
├── test_pathfinding.c      # Movement, stuck detection
├── test_map.c              # Grid bounds, tile state
├── test_snapshot.c         # Double-buffer, atomicity
├── test_integration.c      # End-to-end scenarios
└── test_runner.c           # Main test harness
```

### Test Macros:
```c
#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s:%d - %s\n", __FILE__, __LINE__, msg); \
            return -1; \
        } \
    } while(0)

#define TEST_ASSERT_EQ(a, b, msg) TEST_ASSERT((a) == (b), msg)
#define TEST_ASSERT_NE(a, b, msg) TEST_ASSERT((a) != (b), msg)
#define TEST_ASSERT_NULL(ptr, msg) TEST_ASSERT((ptr) == NULL, msg)
#define TEST_ASSERT_NOT_NULL(ptr, msg) TEST_ASSERT((ptr) != NULL, msg)
```

### Running Tests:
```bash
# Build with tests
cmake -B build -DPATIKA_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Run all tests
./build/patika_tests

# Run with valgrind
valgrind --leak-check=full ./build/patika_tests

# Run with AddressSanitizer
cmake -B build -DPATIKA_ENABLE_ASAN=ON
cmake --build build
./build/patika_tests
```

---

## Next Actions (Priority Order)

1. Implement logging system (patika_log.c)
2. Fix agent_pool_init() bug
3. Write 5 unit tests for agent pool
4. Add occupancy tracking to MapTile
5. Implement CMD_REMOVE_AGENT handler
6. Write integration test (100 agents, 10 seconds)
7. Run valgrind, fix any leaks

---

## Contributing Guidelines

1. Code Style: K&R brace style, 4-space indent, 80-char lines
2. Commits: Atomic, descriptive messages ("Fix: agent_pool_free parameter bug")
3. Testing: Add unit test for every bug fix
4. Documentation: Update README.md if public API changes
5. Performance: Profile before/after optimization claims

---

## Support & Resources

- Issues: GitHub Issues (when published)
- Discussions: GitHub Discussions
- Email: pytgion@gmail.com
- Documentation: TODO: DOCS HERE

---

**Next Review:** After Milestone 2 (Week 4)
