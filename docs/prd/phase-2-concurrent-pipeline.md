# Phase 2: Concurrent Pipeline -- Lock-Free Multithreading

**Spec Reference:** Section 5 (Pages 10-13)
**Duration:** Weeks 9-16
**Goal:** Decouple network I/O from processing using lock-free inter-thread communication, achieving deterministic latency.

## Deliverables

### 1. Thread Architecture
Split the system into dedicated threads: I/O, Strategy, OMS Stub.
- Use std::jthread (C++20) with cooperative cancellation via std::stop_token
- Pin each thread to a specific CPU core using pthread_setaffinity_np()
- Isolate cores from OS scheduler using isolcpus kernel parameter
- **Acceptance:** Three threads running, each pinned to a dedicated core

### 2. Production SPSC Queue
Upgrade the warm-up SPSC queue to production-grade.
- Cache-line-padded head and tail indices (prevent false sharing)
- Power-of-2-sized array for fast modulo (bitwise AND)
- try_push/try_pop (non-blocking) and push_batch/pop_batch for bulk operations
- Monotonic sequence number per slot for ABA problem avoidance
- **Acceptance:** Throughput > 100M messages/second in isolated benchmark

### 3. Memory Ordering
Implement correct and minimal memory ordering on all atomics.
- Producer: memory_order_release on tail write
- Consumer: memory_order_acquire on tail read
- Relaxed ordering on local-only operations
- Never use seq_cst unless provably necessary
- **Acceptance:** Correct behavior under TSAN; no unnecessary seq_cst

### 4. Production Object Pool
Provide allocation-free object creation for MarketDataEvent and OrderRequest.
- Templated ObjectPool<T, N> with contiguous pre-allocated array
- Intrusive free-list (union with object's first field)
- O(1) allocate/deallocate
- Debug-mode checks for double-free and use-after-free
- **Acceptance:** Zero allocations during steady-state (verified with custom operator new)

### 5. Pipeline Plumbing
Wire I/O thread to strategy thread via SPSC queue.
- I/O thread: recv() -> parse() -> pool.allocate() -> fill event -> queue.push()
- Strategy thread: queue.pop() -> update_book() -> evaluate_strategy() -> pool.deallocate()
- Backpressure detection: log warning if queue > 80% full
- **Acceptance:** End-to-end pipeline latency < 2us at p99

### 6. Latency Measurement Infrastructure
Instrument the pipeline with per-message latency tracking.
- TSC timestamp at recv() completion and at processing completion
- Measurements stored in pre-allocated ring buffer of uint64_t
- Compute p50/p90/p99/p99.9 every N seconds using std::nth_element
- **Acceptance:** Latency histogram reporting p50/p90/p99/p99.9 every 10 seconds

### 7. Clean Shutdown
Graceful shutdown with no data loss.
- std::stop_token-based cooperative cancellation
- Drain queues before thread exit
- **Acceptance:** Clean shutdown verified, no data loss, no ASAN/TSAN warnings

### 8. Simulator Pipeline Integration
Verify the simulator feed works correctly through the concurrent pipeline.
- Simulator feed runs on the I/O thread, pushes to SPSC queue identically to live feed
- Strategy thread processes simulator events with the same latency characteristics
- Enables full pipeline testing without exchange connectivity
- **Acceptance:** Simulator data flows through the full pipeline; latency measurements are valid

## Training Topics
- std::atomic and memory ordering (acquire/release/relaxed/seq_cst)
- False sharing: why it happens, how to detect, how to fix
- std::jthread and std::stop_token (new in C++20)
- ABA problem in lock-free data structures
- Cache coherency protocols (MESI)
