# Phase 1: Core Feed Handler -- Single-Threaded Foundation

**Spec Reference:** Section 4 (Pages 7-10)
**Duration:** Weeks 3-8
**Goal:** Connect to a live exchange WebSocket, parse market data at wire speed, and reconstruct an L2 order book in a single thread.

## Deliverables

### 1. WebSocket Client
Establish a persistent WebSocket connection to Binance or Bybit and receive raw market data frames.
- Use Boost.Beast (built on Boost.Asio)
- Configure TCP_NODELAY to disable Nagle's algorithm
- Pre-allocated 64KB read buffer (no per-message allocation)
- Handle ping/pong heartbeats
- **Acceptance:** Connects to Binance and receives live BTC/USDT depth stream

### 2. Message Parser
Deserialize raw JSON market data into internal C++ structs at the lowest possible cost.
- Use simdjson On Demand API for zero-copy JSON traversal
- Parse into pre-allocated MarketDataEvent struct
- Use std::string_view (never std::string) for symbol names
- Fixed-point integer arithmetic for prices and quantities (int64_t, scale 10^8)
- **Acceptance:** Parse time < 1us per message (benchmarked)

### 3. MarketDataEvent Struct
Define the core data structure that carries parsed market data through the pipeline.
- POD struct, 64 or 128 bytes, cache-line aligned with alignas(64)
- Fields: timestamp, symbol_id, side, price, quantity, sequence_number, event_type
- Explicit padding to avoid compiler-inserted padding
- **Acceptance:** sizeof and alignof verified; fits in 1-2 cache lines

### 4. Internal Order Book
Maintain the top N levels of the bid and ask sides for one instrument.
- Two std::array<PriceLevel, 20> for bids and asks (fixed-capacity, stack-allocated)
- Linear scan for updates (faster than tree for small N due to cache locality)
- Denormalized best_bid/best_ask for O(1) top-of-book access
- **Acceptance:** Book verified correct against REST snapshot

### 5. Sequence Tracking
Detect dropped or out-of-order messages from the exchange.
- Track last sequence number per stream
- On gap: log warning, request snapshot resync via REST API
- Use std::expected or Result<T,E> for error propagation (no exceptions on hot path)
- **Acceptance:** Detects and recovers from sequence gaps

### 6. Timestamping
Stamp every event with a high-resolution local timestamp for latency measurement.
- Read CPU timestamp counter directly via __rdtsc() or std::chrono::steady_clock
- Calibrate TSC ticks to nanoseconds at startup
- Measure wire-to-state latency
- **Acceptance:** Wire-to-book-update latency < 5us (measured with TSC)

### 7. Logging (v1)
Log system events without blocking the hot path.
- Synchronous, fire-and-forget logger writing to pre-allocated circular char buffer
- Background flush to disk every 100ms
- fmt::format_to() for zero-allocation formatting
- Temporary solution, replaced in Phase 5
- **Acceptance:** Log formatting does not allocate (verified with custom global operator new)

### 8. Zero-Allocation Verification
Prove the hot path never allocates after initialization.
- Custom global operator new that counts calls
- Assert count is zero during steady-state operation
- **Acceptance:** Zero heap allocations after initialization

### 9. Simulator Feed
Implement `ExchangeFeed<Simulator>` as a venue specialization that replays market data without connecting to a real exchange.
- Reads from recorded raw data (JSONL from data recorder) or generates synthetic market data
- Pushes events through the same pipeline as live feeds
- Strategy and book code cannot distinguish simulator from real venue
- Enables testing the full read path without exchange connectivity
- **Acceptance:** Simulator feed produces identical book state as replaying the same data through the live Bybit feed

## Training Topics
- std::string_view lifetime safety
- alignas(N) and cache line alignment
- Fixed-point arithmetic for financial data
- simdjson On Demand API internals
- Boost.Beast/Asio async model
