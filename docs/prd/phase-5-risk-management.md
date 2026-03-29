# Phase 5: Risk Management & Production Hardening

**Spec Reference:** Section 8 (Pages 19-21)
**Duration:** Weeks 35-42
**Goal:** Make the system safe to run with real money by adding comprehensive risk controls, async logging, monitoring, and crash recovery.

## Deliverables

### 1. Pre-Trade Risk Gateway
Validate every order against risk limits before it reaches the exchange.
- Synchronous check on warm path (before exchange submission)
- Checks: max order size, max position per instrument, max total exposure, max orders/second (rate limit), price sanity (reject orders > X% from mid-price)
- All limits in RiskConfig struct with atomic reads for runtime updates
- Rejected orders log reason and notify strategy
- **Acceptance:** All risk checks pass comprehensive test suite (including edge cases: max-int positions, zero-price orders)

### 2. Kill Switch
Immediate, unconditional mechanism to cancel all orders and halt trading.
- Global std::atomic<bool> kill_switch flag
- When set: (1) cancel all pending orders via bulk cancel API, (2) reject all new orders, (3) notify all strategies to wind down
- Triggers: max daily loss exceeded, anomaly detection, manual signal (SIGUSR1)
- Also implement "soft" kill: stop new orders but let existing ones fill
- Checked independently by OMS before every exchange API call (not a callback into strategy)
- **Acceptance:** Kill switch tested by triggering mid-trade on testnet; verified clean shutdown

### 3. Async Logger (MPSC)
Replace Phase 1 logging with high-throughput, zero-contention async logger.
- All threads push to Multi-Producer Single-Consumer lock-free queue
- Dedicated logger thread drains queue and writes to disk
- Log messages pre-formatted by producer (fmt::format_to into stack buffer)
- Fixed-size blocks moved into queue
- Binary log format for hot-path logs (separate tool decodes for human reading)
- **Acceptance:** Logging does not add measurable latency to hot path

### 4. Latency Histogram
Track and report latency percentiles for every pipeline stage.
- Custom histogram with logarithmically-spaced buckets (1ns to 1s)
- O(1) per measurement (array index operation, no allocation)
- Compute p50/p90/p99/p99.9 every 10 seconds
- Export to Prometheus endpoint
- **Acceptance:** Histogram reporting verified; exported metrics match expected values

### 5. State Snapshots
Save complete system state for fast recovery.
- Memory-mapped files (mmap) for position and order state
- Periodic msync to disk; also on shutdown
- On restart: re-map file and resume from last snapshot
- CRC32 checksum to detect corruption
- **Acceptance:** Process killed with SIGKILL, correct recovery from snapshot verified

### 6. Anomaly Detection
Automatically detect and respond to misbehavior.
- Monitor: position divergence (local vs exchange), order rate exceeding expectations, strategy P&L exceeding daily bounds, feed latency exceeding threshold
- Each anomaly has configured response: log-only, soft-kill, or hard-kill
- **Acceptance:** Each anomaly type triggered in test and correct response verified

### 7. Simulator Stress Testing
Use the simulator to test risk controls under extreme conditions that can't be safely tested on testnet.
- Simulate flash crash (price drops 50% in 1 second)
- Simulate exchange outage mid-position (feed goes silent, orders pending)
- Simulate runaway strategy (rapid-fire order submission)
- Simulate position divergence (simulator reports different position than local tracker)
- Verify kill switch, anomaly detection, and risk gateway all respond correctly
- **Acceptance:** All stress scenarios trigger the correct risk response; system recovers or halts cleanly

### 8. Mainnet Graduation
Do not go live until all criteria met.
- All risk checks pass test suite
- Kill switch tested mid-trade on testnet AND simulator
- 7+ consecutive days on simulator with no crashes, no position divergence, no unhandled errors
- 7+ consecutive days on testnet with the same criteria
- State snapshot/recovery tested with SIGKILL
- Daily loss limit tested by simulating exceeding it via simulator
- **Acceptance:** All graduation criteria documented and signed off

## Training Topics
- Pre-trade risk design philosophy: defense in depth, fail-safe defaults
- MPSC lock-free queue design (CAS on tail pointer)
- Binary logging vs text logging: write-time vs read-time formatting cost
- mmap for state persistence: the struct IS the on-disk format
- Anomaly detection patterns for trading systems
