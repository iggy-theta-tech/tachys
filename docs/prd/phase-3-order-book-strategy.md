# Phase 3: Order Book Engine & Strategy Logic

**Spec Reference:** Section 6 (Pages 14-16)
**Duration:** Weeks 17-24
**Goal:** Build a correct, high-performance order book and implement initial market-making and arbitrage detection strategies.

## Deliverables

### 1. L2 Order Book Engine
Maintain a complete Level 2 order book with guaranteed correctness.
- Separate bid/ask sides using sorted std::vector<PriceLevel> (flat, cache-friendly)
- Denormalized best_bid/best_ask updated on every modification
- apply_snapshot() and apply_delta() methods
- Periodic checksum/CRC verification against exchange-provided checksums
- **Acceptance:** Passes checksum verification against exchange reference (Binance or Bybit)

### 2. Market-Making Strategy
Basic market-making that quotes two-sided markets around fair-value.
- Fair-value estimate (initially midpoint)
- Configurable number of quote levels on each side
- Spread and skew parameters
- Inventory skew: widen ask / tighten bid as position grows long (and vice versa)
- All parameters in cache-line-aligned config struct, updatable at runtime via atomic reads
- **Acceptance:** Generates correct quote updates at > 10,000 updates/second

### 3. Arbitrage Detector
Detect cross-exchange arbitrage opportunities.
- Compare best_bid[venue_A] against best_ask[venue_B] (and vice versa)
- Account for fees on both sides
- Signal only when expected profit exceeds configurable threshold
- Pure function: takes two BookSnapshot structs, returns std::optional<ArbitrageSignal>
- No allocations, no side effects
- **Acceptance:** Correctly identifies known historical opportunities from replayed data

### 4. Strategy Abstraction (CRTP)
Allow multiple strategies compiled into the same binary without virtual dispatch.
- CRTP base: Strategy<Derived> with static dispatch via static_cast
- Concrete strategies: MarketMaker, Arbitrageur inherit from Strategy<T>
- Compile-time polymorphism, zero runtime cost
- **Acceptance:** Zero virtual function calls on hot path (verified with perf branch-mispredict counter)

### 5. Signal/Order Request Generation
Minimal struct carrying trade signals from strategy to OMS.
- OrderRequest: {symbol_id, side, price, quantity, order_type, time_in_force, strategy_id, timestamp}
- Allocated from ObjectPool, pushed to OMS queue via SPSC
- enum class for all categorical fields
- **Acceptance:** Struct fits in one cache line; passes through queue without allocation

### 6. Book Update Performance
Optimize book updates for minimal latency.
- **Acceptance:** Book update latency < 200ns for a single level update (benchmarked)

### 7. Strategy Testing via Simulator
Validate strategies against the simulator feed with recorded or synthetic data.
- Run market-making strategy against simulator replaying historical Bybit data
- Verify quote generation, inventory skew behavior, and signal correctness
- Strategies cannot distinguish simulator from live venue
- **Acceptance:** Market-making strategy produces expected quotes for known market scenarios; arbitrage detector identifies known opportunities in replayed data

## Training Topics
- CRTP (Curiously Recurring Template Pattern): compile-time polymorphism
- constexpr and compile-time computation
- Runtime-updatable config via atomic reads (no locks)
- Market-making fundamentals: spread, skew, inventory management
- Order book microstructure
