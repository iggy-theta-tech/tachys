# Phase 0: Environment, Tooling & C++ Re-Immersion

**Spec Reference:** Section 3 (Pages 6-7)
**Duration:** Weeks 1-2
**Goal:** Set up a professional C++ development environment and rebuild muscle memory.

## Technical Decisions

- **Platform:** macOS (ARM) for development. Linux EC2 box added in Phase 2 for perf/thread pinning. Platform abstraction header (`platform.h`) from day one.
- **Compiler:** Homebrew Clang (LLVM 17+) for full C++20/23 support (std::expected, concepts, constexpr if).
- **Build system:** CMake 3.28+ with FetchContent for dependencies. Migrate to Conan in Phase 2 or 3 when dependency count grows.
- **Code style:** LLVM `.clang-format` base.
- **Debugger:** lldb on macOS for quick debugging. gdb in Docker (Linux container) for Linux-native debugging.
- **Docker:** Linux container for gdb, sanitizers, valgrind/cachegrind. Not for perf (PMU counters need bare metal).
- **CI:** GitHub Actions, Linux runners. Set up in Phase 0.

## Project Structure

Each directory is its own CMake library with its own CMakeLists.txt and explicit dependency declarations.

```
tachys/
  CMakeLists.txt                    # Root CMake, defines presets, pulls FetchContent deps
  CMakePresets.json                 # Debug, Release, Benchmark presets
  .clang-format                    # LLVM base
  .gitignore
  Dockerfile                       # Linux dev container (gdb, sanitizers, valgrind)
  .github/
    workflows/
      ci.yml                       # GitHub Actions CI pipeline
  src/
    core/                          # Types, platform abstractions, utilities
      CMakeLists.txt
    feed/                          # WebSocket, parser, market data
      CMakeLists.txt
    book/                          # Order book engine
      CMakeLists.txt
    strategy/                      # Market-making, arbitrage
      CMakeLists.txt
    oms/                           # Order management, execution
      CMakeLists.txt
    risk/                          # Pre-trade risk, kill switch
      CMakeLists.txt
    infra/                         # Logging, metrics, config
      CMakeLists.txt
  tests/
    core/
    feed/
    book/
    strategy/
    oms/
    risk/
    infra/
  benchmarks/
    core/
    feed/
    book/
    strategy/
    oms/
    risk/
    infra/
  exercises/                       # Warm-up exercises (throwaway learning code)
```

## Deliverables

### 1. CMake Project Setup

**What:** Root CMakeLists.txt with presets and the full directory structure with per-component CMakeLists.txt files.

**How:**
- CMakePresets.json with three presets:
  - `debug`: `-O0 -g -fsanitize=address,undefined -DTACHYS_DEBUG=1`
  - `release`: `-O3 -march=native -flto -DNDEBUG`
  - `benchmark`: `-O3 -g` (profiling with symbols)
- Root CMakeLists.txt:
  - `cmake_minimum_required(VERSION 3.28)`
  - `project(tachys LANGUAGES CXX)`
  - `set(CMAKE_CXX_STANDARD 23)` / `set(CMAKE_CXX_STANDARD_REQUIRED ON)`
  - `set(CMAKE_EXPORT_COMPILE_COMMANDS ON)` (for clangd)
  - FetchContent blocks for all dependencies
  - `add_subdirectory()` for each src/ component
  - `enable_testing()` and test/benchmark subdirectories
- Each component CMakeLists.txt:
  - Defines a static library target (e.g., `tachys_core`, `tachys_feed`)
  - Declares `target_link_libraries()` with explicit dependencies
  - No circular dependencies allowed

**Dependencies via FetchContent (pinned versions):**
- Boost 1.84+ (Asio, Beast)
- Google Test (latest stable)
- Google Benchmark (latest stable)
- fmt (latest stable)
- simdjson (latest stable)

**Acceptance:**
- `cmake --preset debug && cmake --build --preset debug` compiles cleanly with zero warnings (`-Wall -Wextra -Wpedantic -Werror`)
- `ctest` runs with no test targets yet (but the framework works)
- `compile_commands.json` generated in build directory

### 2. Platform Abstraction Header

**What:** `src/core/include/tachys/platform.h` providing cross-platform primitives.

**How:**
- Timestamp reading:
  - Linux x86: `__rdtsc()` intrinsic
  - macOS ARM: `std::chrono::steady_clock::now()`
  - Common interface: `tachys::clock::now()` returning uint64_t nanoseconds
- Thread pinning:
  - Linux: `pthread_setaffinity_np()`
  - macOS: advisory `pthread_set_qos_class_self()` (best effort)
  - Common interface: `tachys::thread::pin_to_core(int core_id)`
- Cache line size:
  - `constexpr size_t CACHE_LINE_SIZE = 64;` (same on x86 and Apple Silicon)
  - `#define TACHYS_CACHELINE_ALIGNED alignas(tachys::CACHE_LINE_SIZE)`
- Platform detection via `#ifdef __linux__`, `#ifdef __APPLE__`, `#ifdef __x86_64__`, `#ifdef __aarch64__`

**Acceptance:**
- Compiles on macOS ARM with Homebrew Clang
- Compiles in Docker Linux container
- `tachys::clock::now()` returns monotonically increasing nanosecond timestamps
- `tachys::thread::pin_to_core()` succeeds on Linux, logs warning on macOS

### 3. Docker Development Container

**What:** Dockerfile for Linux development environment with gdb, sanitizers, valgrind.

**How:**
- Base image: Ubuntu 22.04
- Install: GCC 13+, Clang 17+, gdb, valgrind, cmake 3.28+, ninja-build
- Mount project directory as volume
- Makefile or shell script shortcuts: `make docker-build`, `make docker-test`, `make docker-gdb`

**Acceptance:**
- `docker build` completes successfully
- Project compiles inside container with both GCC and Clang
- gdb attaches to a running binary inside container
- valgrind (cachegrind) produces a cache report on a test binary
- AddressSanitizer and UBSan work inside container

### 4. CI Pipeline (GitHub Actions)

**What:** CI that builds and tests on every push/PR.

**How:**
- Workflow file: `.github/workflows/ci.yml`
- Matrix build: GCC 13 + Clang 17 on Ubuntu
- Steps:
  1. Checkout
  2. Install dependencies (apt-get for compiler, cmake)
  3. Configure with debug preset
  4. Build with `-Wall -Wextra -Wpedantic -Werror`
  5. Run tests (ctest)
  6. Run sanitizers (ASAN, UBSAN)
- Later phases add: benchmark regression, clang-tidy, coverage

**Acceptance:**
- CI passes on a clean checkout
- CI fails on a deliberate compilation error (verified)
- CI fails on a deliberate ASAN violation (verified)

### 5. IDE/Editor Configuration

**What:** clangd setup with C++23-aware autocompletion.

**How:**
- `.clangd` config file pointing to `compile_commands.json`
- `.clang-format` with LLVM base style
- `.clang-tidy` with a conservative initial rule set (modernize-*, bugprone-*, performance-*)
- Format-on-save configured (editor-agnostic, documented in README)

**Acceptance:**
- clangd provides autocompletion for C++23 features (std::expected, concepts)
- clang-format formats on save
- clang-tidy produces zero warnings on the initial codebase

### 6. Exercise: RAII Socket Wrapper

**What:** `exercises/raii_socket.cpp` -- TCP socket class with RAII and move semantics.

**How:**
- Class `TcpSocket` that opens a socket in constructor, closes in destructor
- Move constructor and move assignment operator (both `noexcept`)
- Deleted copy constructor and copy assignment
- `send(std::span<const std::byte>)` and `recv(std::span<std::byte>)` methods
- Connect to a public echo server (e.g., tcpbin.com) and send/receive a message
- Google Test: verify move leaves source in valid empty state, verify no resource leak

**Acceptance:**
- Sends and receives a message successfully
- ASAN reports zero leaks
- Move semantics work correctly (source socket is invalidated)
- Double-close is impossible by construction

### 7. Exercise: Lock-Free SPSC Queue

**What:** `exercises/spsc_queue.cpp` -- fixed-capacity single-producer single-consumer ring buffer.

**How:**
- Template: `SpscQueue<T, Capacity>` where Capacity is power of 2
- `std::atomic<size_t>` for head and tail indices
- Producer: `memory_order_release` on tail write
- Consumer: `memory_order_acquire` on tail read
- `try_push(const T&)` returning bool, `try_pop(T&)` returning bool
- Head and tail on separate cache lines (`alignas(64)`)
- Google Benchmark:
  - Producer and consumer on separate threads (pinned where possible)
  - Measure throughput (messages/second) and latency (ns/message)

**Acceptance:**
- < 50ns round-trip latency (benchmarked)
- TSAN reports zero data races
- Correct behavior under stress (millions of messages, no lost/duplicated)

### 8. Exercise: Object Pool Allocator

**What:** `exercises/object_pool.cpp` -- fixed-size pool with intrusive free-list.

**How:**
- Template: `ObjectPool<T, N>`
- Pre-allocates N objects in contiguous array at construction
- Free-list: intrusive singly-linked list using union with object's first field
- `T* allocate()` -- pops from free-list in O(1), returns nullptr if exhausted
- `void deallocate(T*)` -- pushes back to free-list in O(1)
- Debug-mode checks:
  - Double-free detection (poison freed slots)
  - Use-after-free detection (check poison on allocate)
  - Allocation from wrong pool detection (bounds check)
- Google Benchmark: compare against `std::allocator` (new/delete)

**Acceptance:**
- Zero heap allocations after construction (verified with custom operator new)
- Benchmarks show measurable improvement over std::allocator
- Debug checks catch double-free and use-after-free in test

### 9. Exercise: simdjson Parsing

**What:** `exercises/simdjson_parse.cpp` -- parse a Bybit WebSocket order book snapshot.

**How:**
- Download a sample Bybit WebSocket depth message (save as test fixture JSON file)
- Parse with simdjson On Demand API
- Extract bids and asks into `std::array<PriceLevel, 20>`
- PriceLevel: `{int64_t price, int64_t quantity}` using fixed-point (scale 10^8)
- Use `std::string_view` for all string access (never allocate std::string)
- Google Benchmark: measure parse time per message

**Acceptance:**
- Parse time < 1us per message (benchmarked)
- Parsed values match expected values from fixture
- Zero heap allocations during parse (verified with custom operator new)
- Fixed-point prices compare correctly (no floating-point comparison issues)

## CI Pipeline Details

The CI pipeline runs on every push and PR:

```yaml
Build Matrix:
  - GCC 13, Ubuntu 22.04
  - Clang 17, Ubuntu 22.04

Steps:
  1. Build (debug preset, -Werror)
  2. Unit tests (ctest)
  3. Sanitizers (ASAN + UBSAN)
  4. clang-tidy (warning only in Phase 0, error in Phase 1+)
```

Future additions per phase:
- Phase 1: clang-tidy as error
- Phase 2: TSAN for concurrency tests
- Phase 8: benchmark regression detection

## Training Topics
- Modern C++20/23 delta: concepts, std::expected, constexpr containers, std::jthread
- std::atomic refresher with memory ordering (acquire/release/relaxed)
- std::span as modern replacement for pointer+length
- CMake modern practices (presets, FetchContent, generator expressions)
- simdjson On Demand API internals
- LLVM toolchain: clangd, clang-format, clang-tidy

## Notes
- Migrate from FetchContent to Conan in Phase 2 or 3 when dependency count grows
- Linux EC2 box deferred to Phase 2 (first need for perf and enforced thread pinning)
