# Phase 1: Core Feed Handler -- Single-Threaded Foundation

**Spec Reference:** Section 4 (Pages 7-10)
**Duration:** Weeks 3-8
**Goal:** Connect to a live exchange WebSocket, parse market data at wire speed, and reconstruct an L2 order book in a single thread.

## Technical Decisions

- **Primary exchange:** Bybit. Exchange abstraction designed for extensibility from day one. Binance added as second exchange.
- **Primary pair:** BTC/USDT for development and testing. Adding new pairs should be trivial (config-driven, not code change).
- **Networking:** Boost.Beast for both WebSocket and REST (single async model via Asio, shared connection pooling).
- **Connection management:** Full connection manager from the start (health monitoring, automatic reconnection, failover).
- **Book depth:** Compile-time configurable via template parameter. Default 20 levels.
- **Data recording:** Raw WebSocket message dump to disk from Phase 1. Simple format now, replaced with proper binary logger in Phase 8. Ensures historical data is available for ML training.
- **Allocation tracking:** Debug-only assertions plus a lightweight Release-mode metric counter for production monitoring.
- **Simulator:** `ExchangeFeed<Simulator>` from Phase 1 as a venue specialization. Replays recorded data or generates synthetic market data. Strategy/book code cannot distinguish it from live. Phase 4 adds `ExchangeGateway<Simulator>` for full paper trading with fake money.

## Deliverables

### 1. Exchange Abstraction Layer

**What:** `src/feed/` -- venue-agnostic interface that Bybit (and later Binance) implements.

**How:**
- `ExchangeFeed<Venue>` CRTP base in `src/feed/include/tachys/exchange_feed.h`
- Derived class implements:
  - `connect()` -- establish WebSocket connection
  - `subscribe(symbol_id)` -- subscribe to depth stream
  - `parse_message(std::string_view raw)` -- parse venue-specific JSON into MarketDataEvent
  - `request_snapshot(symbol_id)` -- REST call for book snapshot resync
- Venue-specific config: endpoint URLs, subscription message format, field names, checksum algorithm
- Config loaded at startup, not hardcoded
- Adding a new exchange: implement a new specialization of `ExchangeFeed<NewVenue>`, add config. No changes to strategy/book code.

**Acceptance:**
- Bybit specialization compiles and connects
- A second venue stub (e.g., `ExchangeFeed<BinanceStub>`) compiles with a trivial implementation, proving the abstraction works
- No venue-specific types leak outside of feed/

### 2. Connection Manager

**What:** `src/feed/` -- robust WebSocket connection lifecycle management.

**How:**
- Class `ConnectionManager` owning the Boost.Beast WebSocket stream
- States: Disconnected -> Connecting -> Connected -> Subscribing -> Active -> Reconnecting
- Health monitoring:
  - Track last message timestamp. If no message received for configurable timeout (e.g., 30s), trigger reconnect.
  - Ping/pong heartbeat handling per exchange protocol
- Reconnection:
  - Exponential backoff: 100ms, 200ms, 400ms, ... capped at 30s
  - On reconnect: re-subscribe to all streams, request snapshot to rebuild book
  - Max reconnection attempts configurable (default: unlimited)
- Metrics: connection uptime, reconnection count, last disconnect reason
- All state transitions logged

**Acceptance:**
- Recovers from a simulated disconnect (kill the WebSocket, verify reconnection and book rebuild)
- Exponential backoff verified in test (mock transport)
- No resource leaks across reconnection cycles (ASAN verified)
- Health check triggers reconnect when feed goes silent

### 3. WebSocket Client

**What:** `src/feed/` -- Boost.Beast WebSocket client integrated with ConnectionManager.

**How:**
- Built on Boost.Beast over Boost.Asio
- TLS support (wss://) via Boost.Beast SSL stream
- `TCP_NODELAY` enabled (disable Nagle's algorithm)
- Pre-allocated read buffer: 64KB fixed buffer, reused across messages
- Asio io_context running on dedicated thread
- Message callback: invokes parser on each complete frame

**Acceptance:**
- Connects to Bybit wss:// endpoint and receives live BTC/USDT depth stream
- TCP_NODELAY verified (getsockopt check in test)
- Read buffer is reused, not reallocated (verified with allocation counter)

### 4. REST Client

**What:** `src/feed/` -- Boost.Beast HTTP client for snapshot resyncs and later order submission.

**How:**
- Built on Boost.Beast HTTP over Asio (same io_context as WebSocket)
- Persistent connection pool: pre-open N connections, reuse across requests
- TLS support
- JSON response parsing via simdjson
- Used for:
  - Order book snapshot requests (GET)
  - Later (Phase 4): order submission (POST with authentication)
- Authentication hooks: virtual method for signing requests (exchange-specific)

**Acceptance:**
- Fetches Bybit order book snapshot via REST
- Connection is reused across multiple requests (no TCP handshake per call)
- Response parsed correctly with simdjson

### 5. Message Parser

**What:** `src/feed/` -- deserialize raw JSON into MarketDataEvent.

**How:**
- simdjson On Demand API for zero-copy JSON traversal
- Parse directly into pre-allocated MarketDataEvent struct
- Bybit-specific field mapping (configurable per venue):
  - Map venue field names to internal fields
  - Handle venue-specific quirks (timestamp format, price format)
- Price/quantity conversion to fixed-point int64_t (scale 10^8)
- Use `std::string_view` for symbol names (never std::string)
- Symbol mapping: string symbol (e.g., "BTCUSDT") -> uint16_t symbol_id via pre-built lookup table at startup

**Acceptance:**
- Parse time < 1us per message (benchmarked with Google Benchmark)
- Parsed values match expected values from test fixtures
- Zero heap allocations during parse (verified with allocation counter)
- Fixed-point price arithmetic: 43521.75 stored as 4352175000000 (verified)

### 6. MarketDataEvent Struct

**What:** `src/core/include/tachys/types.h` -- core data structure for parsed market data.

**How:**
- POD struct, cache-line aligned:
  ```
  struct alignas(64) MarketDataEvent {
      uint64_t timestamp;         // nanoseconds since epoch
      uint64_t sequence_number;
      int64_t  price;             // fixed-point, scale 10^8
      int64_t  quantity;          // fixed-point, scale 10^8
      uint16_t symbol_id;
      uint8_t  side;              // enum class Side : uint8_t { Bid, Ask }
      uint8_t  event_type;        // enum class EventType : uint8_t { Snapshot, Delta, Trade }
      uint8_t  venue_id;          // enum class Venue : uint8_t { Bybit, Binance, ... }
      uint8_t  padding[27];       // explicit padding to 64 bytes
  };
  ```
- `static_assert(sizeof(MarketDataEvent) == 64)` -- fits in one cache line
- `static_assert(std::is_trivially_copyable_v<MarketDataEvent>)`
- All enum fields use `enum class` with explicit underlying type
- Fixed-point constants: `constexpr int64_t PRICE_SCALE = 100'000'000;`

**Acceptance:**
- sizeof == 64, alignof == 64 (static_assert)
- Trivially copyable (static_assert)
- No implicit padding (verified with offsetof checks)

### 7. Internal Order Book

**What:** `src/book/` -- L2 order book for one instrument.

**How:**
- Template: `OrderBook<size_t Depth = 20>`
- Two `std::array<PriceLevel, Depth>` for bids and asks
- `PriceLevel`: `{int64_t price, int64_t quantity}`
- Sorted: bids descending by price, asks ascending
- Linear scan for updates (for Depth < 50, faster than tree due to cache locality)
- Denormalized `best_bid` / `best_ask` updated on every modification for O(1) top-of-book
- Methods:
  - `apply_snapshot(std::span<const PriceLevel> bids, std::span<const PriceLevel> asks)`
  - `apply_delta(Side side, int64_t price, int64_t quantity)` -- quantity 0 means remove level
  - `best_bid()` / `best_ask()` -- O(1) accessors
  - `mid_price()` -- (best_bid + best_ask) / 2
  - `spread()` -- best_ask - best_bid
- Checksum verification: method to compute book checksum per exchange algorithm, compare against exchange-provided checksum

**Acceptance:**
- Book verified correct against REST snapshot (fetch snapshot, apply, compare)
- Checksum matches Bybit's checksum for known test data
- apply_delta for a single level < 200ns (benchmarked)
- Zero heap allocations during any operation

### 8. Sequence Tracking

**What:** `src/feed/` -- detect dropped or out-of-order messages.

**How:**
- Track last sequence number per stream (per symbol, per venue)
- On each message: compare sequence_number with expected (last + 1)
- If gap detected:
  1. Log warning with gap details (expected vs received)
  2. Mark book as stale
  3. Request snapshot resync via REST
  4. Resume normal processing after snapshot applied
- If duplicate (sequence <= last): drop silently, log at debug level
- Use `std::expected<MarketDataEvent, FeedError>` for error propagation (no exceptions)
- `FeedError`: enum class with `SequenceGap`, `ParseError`, `ConnectionLost`, `ChecksumMismatch`

**Acceptance:**
- Detects a simulated sequence gap (skip a message in test) and triggers resync
- Recovers from gap: book matches REST snapshot after recovery
- No exceptions thrown on any error path

### 9. Timestamping

**What:** `src/core/` -- high-resolution timestamps via platform abstraction.

**How:**
- Uses `tachys::clock::now()` from platform.h
- Stamp at two points:
  1. Entry to recv() callback (wire arrival time)
  2. Completion of book update (processing complete time)
- Both timestamps stored in MarketDataEvent (wire_timestamp, process_timestamp)
- Calibration at startup: measure clock resolution, log it
- Latency = process_timestamp - wire_timestamp

**Acceptance:**
- Wire-to-book-update latency < 5us (measured and reported)
- Timestamps are monotonically increasing
- Clock resolution logged at startup

### 10. Logging (v1)

**What:** `src/infra/` -- synchronous buffered logger for Phase 1. Replaced in Phase 5.

**How:**
- Pre-allocated circular char buffer (configurable size, default 1MB)
- `fmt::format_to()` to format directly into buffer (zero allocation)
- Background flush: timer fires every 100ms, writes buffer to disk
- Log levels: TRACE, DEBUG, INFO, WARN, ERROR, FATAL
- Compile-time level filtering: TRACE/DEBUG compiled out in Release
- Thread-safe: single writer assumed in Phase 1 (single-threaded hot path)
- Log format: `[timestamp] [level] [component] message`

**Acceptance:**
- Log formatting does not allocate (verified with allocation counter)
- Background flush verified (logs appear on disk within 200ms)
- TRACE/DEBUG calls are zero-cost in Release build (compiled out)

### 11. Allocation Tracking

**What:** `src/core/` -- custom global operator new for allocation verification.

**How:**
- Custom `operator new` and `operator delete` that increment/decrement atomic counters
- Debug mode: assert counter delta is zero during steady-state operation
- Release mode: export counter as a metric (atomic load, negligible cost)
- API:
  - `tachys::alloc::reset_counter()` -- reset to zero (call after initialization)
  - `tachys::alloc::get_count()` -- current allocation count since reset
  - `TACHYS_ASSERT_NO_ALLOC(block)` -- macro that resets, runs block, asserts count == 0
- Enabled/disabled via CMake option `TACHYS_TRACK_ALLOCATIONS` (ON by default in Debug/Benchmark, optional in Release)

**Acceptance:**
- Catches a deliberate `new int` on the hot path in debug mode
- Release mode metric increments correctly
- Zero overhead when disabled

### 12. Raw Data Recorder

**What:** `src/infra/` -- simple raw message dump for future ML training data.

**How:**
- Receives raw WebSocket message bytes before parsing
- Writes to a file: one JSON message per line (JSONL format)
- File rotation: new file per hour or per configurable size (default 100MB)
- File naming: `data/raw/<venue>/<symbol>/YYYY-MM-DD-HH.jsonl`
- Buffered writes (write every 1000 messages or every second, whichever comes first)
- Not on the hot path: fire-and-forget from I/O thread to a write buffer
- This is a temporary solution. Phase 8 replaces it with proper binary logging.

**Acceptance:**
- Raw messages written to disk in JSONL format
- File rotation works correctly
- Can replay recorded messages and reconstruct the same book state
- Write buffering does not block the I/O thread

### 13. Simulator Feed

**What:** `src/feed/` -- `ExchangeFeed<Simulator>` venue specialization for testing without exchange connectivity.

**How:**
- Implements the same `ExchangeFeed<Venue>` CRTP interface as Bybit
- Two data source modes:
  1. **Replay mode:** Reads from recorded JSONL files (from the raw data recorder). Parses each line and pushes through the same pipeline as live. Configurable playback speed: 1x real-time, max-speed, or stepped (one message at a time for debugging).
  2. **Synthetic mode:** Generates realistic market data programmatically. Configurable parameters: base price, volatility, spread, update frequency, number of levels. Useful for stress testing and scenarios where you need specific market conditions.
- Uses the same MarketDataEvent struct and pushes to the same SPSC queue (once Phase 2 plumbing is in place; in Phase 1, calls the same single-threaded processing path)
- Implements `request_snapshot()` by reading the first message in the replay file or generating a synthetic snapshot
- Sequence numbers are either replayed from recorded data or generated monotonically in synthetic mode
- Timestamps: in replay mode, uses original timestamps with configurable offset to current time. In synthetic mode, uses `tachys::clock::now()`.

**Acceptance:**
- Replay mode: book state after replaying N messages matches book state from live processing of the same N messages
- Synthetic mode: generates valid market data that passes all book validation (sorted levels, positive quantities, valid prices)
- Switching between Simulator and Bybit requires only a config change, no code change
- No simulator-specific code paths in book, strategy, or OMS components

## Pair Configuration

Adding a new trading pair should require only configuration, not code changes:

```
Symbol config (loaded at startup):
  - symbol: "BTCUSDT"
  - symbol_id: 1
  - venue: Bybit
  - ws_stream: "orderbook.20.BTCUSDT"
  - price_tick: 10000000    (0.1 USDT in fixed-point)
  - qty_tick: 1000          (0.00001 BTC in fixed-point)
  - book_depth: 20
```

Config format: JSON file read at startup. Not hardcoded.

## Training Topics
- std::string_view lifetime safety (dangling views into moved-from buffers)
- alignas(N) and cache line alignment (why 64 bytes, what happens if you cross a line)
- Fixed-point arithmetic for financial data (why not double, scale factor choice)
- simdjson On Demand API: tape vs on-demand, when each is appropriate
- Boost.Beast/Asio async model: io_context, strands, completion handlers
- std::expected for error handling without exceptions
- Connection management patterns in production systems

## Simulator Evolution Across Phases

The simulator grows with the system. It is always a venue specialization, never a special mode.

| Phase | Simulator Capability |
|-------|---------------------|
| 1 | Feed simulation (replay + synthetic market data) |
| 2 | Pipeline integration (simulator data through concurrent pipeline) |
| 3 | Strategy validation (run strategies against simulated feed) |
| 4 | Full paper trading (simulated matching engine, fills, positions, P&L) |
| 5 | Stress testing (flash crashes, outages, runaway strategies) |
| 6 | Multi-venue simulation (cross-venue arb testing) |
| 7 | AI signal validation (AI-driven paper trading) |
| 8 | Simulator fidelity testing (compare simulator vs live behavior) |

## Notes
- Exchange abstraction designed for Binance as second venue (Phase 6 formalizes multi-venue, but the interface is ready)
- Raw data recording is a placeholder. Phase 8 binary logger replaces it with SBE/FlatBuffers format.
- The v1 logger is single-threaded safe only. Phase 5 replaces it with MPSC async logger.
- Simulator is a first-class venue, not a test mode. It shares 100% of the code path with live trading.
