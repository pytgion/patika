# Patika lib

A lock-free agent simulation engine used in project Germ Storm. Manages large-scale agent movement with pathfinding, handles commands from multiple threads, and streams events in real-time. Written in C11.

Patika solves the problem of high CPU load and concurrency issues when managing tens of thousands of moving agents in real-time simulations (like crowd management or multiplayer queries). It achieves this efficiency by treating agents not as heavy, individual objects, but as simple data references, enabling grand-scale operations without the overhead of traditional locking mechanisms.

## Usage

```c
PatikaConfig cfg = {
    .max_agents = 10000,
    .max_barracks = 100,
    .grid_width = 512, .grid_height = 512,
    .command_queue_size = 100000,
    .event_queue_size = 50000
};

PatikaHandle sim = patika_create(&cfg);

// Thread 1: Submit commands
patika_submit_command(sim, &cmd_add_agent);

// Thread 1..N: More commands...

// Main thread: Advance simulation
patika_tick(sim);

// Thread 2: Read world state
const PatikaSnapshot *snap = patika_get_snapshot(sim);

// Any thread: Consume events
patika_poll_events(sim, events, max);

patika_destroy(sim);
```
Multi-threaded producers submit commands via lock-free MPSC queue. Single simulation thread processes commands, updates agent positions via hex pathfinding, and emits events through SPSC queue. Snapshot API provides lock-free reads of world state. Agents have generational IDs to prevent use-after-free.

