# Phase 8: Replay, Backtesting & Performance Lab

**Spec Reference:** Section 11 (Pages 26-27)
**Duration:** Weeks 65-72
**Goal:** Build infrastructure to replay historical data through the live system, enabling backtesting and performance regression testing.

## Deliverables

### 1. Binary Data Logger
Record all market data and order events in compact binary format.
- SBE (Simple Binary Encoding) or FlatBuffers for log format
- Each record: header (timestamp, event_type, length) + event payload
- Write to memory-mapped files in large sequential blocks (4MB) for maximum disk throughput
- Rotate files daily
- **Acceptance:** Binary logs are 10-100x smaller than equivalent JSON; write throughput does not impact hot path

### 2. Replay Harness
Feed historical data through the exact same code path as live trading.
- ReplayFeed replaces WebSocket I/O thread
- Reads from binary log files, pushes events to same SPSC queue
- Configurable speed: 1x real-time, max-speed, or stepped
- Strategy and OMS threads don't know (and don't care) whether data is live or replayed
- "One binary, two modes" design
- **Acceptance:** Replay produces identical strategy signals as live for the same data sequence

### 3. Performance Lab
Systematically profile and optimize the critical path.
- perf stat: measure IPC, cache miss rate, branch misprediction rate
- perf record + perf report: find hot functions
- cachegrind for detailed cache simulation
- Targets: > 2.0 IPC on hot path, < 1% L1d cache miss rate, < 0.5% branch misprediction rate
- Document every optimization with before/after measurements
- **Acceptance:** All performance targets met or documented with explanation of why not

### 4. Regression Testing
Ensure code changes don't degrade performance.
- CI benchmark suite that replays 1 hour of historical data
- Measures p50/p90/p99/p99.9 latency
- CI fails if any percentile degrades by > 10% vs baseline
- Google Benchmark for microbenchmarks of individual components
- Store benchmark results in version control for trend tracking
- **Acceptance:** CI pipeline correctly catches a deliberate performance regression

### 5. Backtest Framework
Run strategies against historical data and measure hypothetical P&L.
- Feed replayed data through strategy and the simulator execution engine (reuses `ExchangeGateway<Simulator>` from Phase 4)
- Configurable fill models: immediate, probabilistic, queue-position-based
- Output: trade log, P&L curve, Sharpe ratio, max drawdown, fill rate
- **Acceptance:** Backtest results are reproducible (same data + same config = same output)

### 6. Simulator vs Live Comparison
Validate simulator fidelity by comparing simulator behavior against live/testnet results.
- Run the same strategy on simulator and testnet simultaneously with the same market data
- Compare: fill rates, execution latency, P&L divergence
- Calibrate simulator fill models based on observed testnet behavior
- **Acceptance:** Simulator P&L tracks within configurable tolerance of testnet P&L over a 7-day period

## Training Topics
- SBE and FlatBuffers: binary serialization for financial systems
- perf tooling: IPC, cache miss analysis, branch prediction
- Performance optimization methodology: measure, hypothesize, change, measure
- Branch-free programming techniques
- CI benchmark design and baseline management
