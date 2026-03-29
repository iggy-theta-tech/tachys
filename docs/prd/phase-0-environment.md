# Phase 0: Environment, Tooling & C++ Re-Immersion

**Spec Reference:** Section 3 (Pages 6-7)
**Duration:** Weeks 1-2
**Goal:** Set up a professional C++ development environment and rebuild muscle memory.

## Deliverables

### 1. CMake Project Setup
Set up CMake 3.28+ with presets for Debug, Release, and Benchmark builds.
- Debug: `-O0 -g -fsanitize=address,undefined`
- Release: `-O3 -march=native -flto -DNDEBUG`
- Benchmark: `-O3 -g` (profiling with symbols)
- Generate `compile_commands.json` for clangd
- Configure `.clang-format` for consistent code style
- **Acceptance:** Project compiles with C++20, runs Google Test and Google Benchmark

### 2. Dependency Management
Install and verify all dependencies via CMake FetchContent or vcpkg. Pin exact versions.
- Boost 1.84+ (for Asio/Beast)
- Google Benchmark
- Google Test
- fmt library
- simdjson
- **Acceptance:** All dependencies resolve and compile cleanly

### 3. Profiling Tools Setup
Install and verify profiling tools.
- perf (verify PMU access with `perf stat` on a simple loop)
- Valgrind (cachegrind/callgrind)
- Optionally Intel VTune
- **Acceptance:** `perf stat` runs successfully on a test binary; cachegrind produces cache miss report on a matrix multiply

### 4. IDE/Editor Configuration
Configure clangd with C++20-aware autocompletion.
- clangd picks up `compile_commands.json`
- Format-on-save with `.clang-format`
- **Acceptance:** Autocompletion works for C++20 features (concepts, std::expected)

### 5. Exercise: RAII Socket Wrapper
Write a class that opens a TCP socket in constructor, closes in destructor. Implement move semantics, delete copy operations. Connect to a public echo server.
- **Concepts:** RAII, move semantics, Linux socket API
- **Acceptance:** Sends and receives a message; no resource leaks under ASAN

### 6. Exercise: Lock-Free SPSC Queue
Implement a fixed-capacity SPSC ring buffer using `std::atomic<size_t>`. Use acquire/release ordering. Benchmark with producer/consumer on separate pinned cores.
- **Concepts:** std::atomic, memory ordering, cache lines
- **Acceptance:** < 50ns round-trip latency on modern hardware (benchmarked)

### 7. Exercise: Object Pool Allocator
Create a fixed-size pool with O(1) allocate/deallocate using intrusive free-list. Zero heap allocation after construction.
- **Concepts:** Custom allocators, intrusive data structures, pool allocation
- **Acceptance:** Benchmarks show improvement over std::allocator; zero post-init allocations

### 8. Exercise: simdjson Parsing
Parse a sample Binance WebSocket order book snapshot with simdjson On Demand API. Extract bids/asks into pre-allocated std::array.
- **Concepts:** Zero-copy JSON parsing, fixed-point arithmetic, std::string_view
- **Acceptance:** Parse time < 1us per message (benchmarked)

## Training Topics
- Modern C++20/23 delta: concepts, std::expected, constexpr containers, modules
- std::atomic refresher with memory ordering
- std::span, std::string_view usage patterns
- CMake modern practices (presets, FetchContent)
