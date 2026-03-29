# Phase 4: Execution Engine & Order Management System

**Spec Reference:** Section 7 (Pages 16-19)
**Duration:** Weeks 25-34
**Goal:** Build the component that sends real orders to exchanges, tracks their lifecycle, and manages positions. This is where the system goes from read-only to read-write.

**Warning:** This is the hard phase. Bugs here can cost real money. Start on testnet and stay on testnet until Phase 5 (Risk Management) is complete.

## Deliverables

### 1. Order Lifecycle FSM
Model the complete lifecycle of an order as a finite state machine.
- States: Created -> PendingNew -> Acknowledged -> PartiallyFilled -> Filled | Cancelled | Rejected
- Transition table as constexpr std::array of valid transitions
- Every state transition logged with timestamps
- Invalid transitions trigger error and position reconciliation
- **Acceptance:** FSM handles all valid transitions and rejects invalid ones

### 2. Exchange Gateway
Abstract exchange-specific REST/WebSocket API behind a clean interface.
- Templated ExchangeGateway<Venue> (CRTP) implementing: submit_order(), cancel_order(), cancel_replace(), query_order()
- Each venue specialization handles specific API format
- Pre-created connection pool (persistent connections, no TCP handshake per order)
- Parse fill reports from WebSocket user data stream
- **Acceptance:** Round-trip order latency (submit to ack) measured and logged, typically 5-50ms for REST

### 3. Position Tracker
Track real-time position, average entry price, and unrealized P&L per instrument.
- Position struct: {int64_t quantity (signed), int64_t avg_entry_price, int64_t realised_pnl, int64_t unrealised_pnl}
- Updated on every fill
- Unrealized P&L recalculated on every book update using current mid-price
- Owned exclusively by OMS thread (no sharing, no atomics needed)
- **Acceptance:** Position tracker agrees with exchange-reported position after 24 hours of continuous testnet trading

### 4. Order ID Management
Generate locally unique order IDs and map to exchange-assigned IDs.
- Per-session monotonic counter for local IDs (uint64_t, never reused)
- Pre-allocated flat_hash_map<local_id, exchange_id> for cross-referencing
- active_orders array (fixed capacity, linear scan, rarely > 20 active)
- Fixed-size char array (char[32]) for exchange IDs (avoid string allocation)
- **Acceptance:** No allocation on order ID creation or lookup

### 5. Fill Handler
Process fill reports, update positions, reconcile with expected state.
- On fill: validate order ID, transition FSM, update position, update P&L, log
- On unexpected fill (unknown order ID): trigger alert, pause trading, force position reconciliation via REST
- **Acceptance:** Unexpected fill handling tested and verified (pauses and reconciles, does not crash)

### 6. Simulator Execution Engine
Implement `ExchangeGateway<Simulator>` with a local matching engine for full paper trading.
- Accepts orders, generates fills with configurable fill models:
  - Immediate fill (100% fill at submitted price)
  - Partial fill (configurable fill rate)
  - Latency injection (configurable delay before ack/fill)
  - Reject simulation (configurable reject rate)
  - Slippage model (price moves between submission and fill)
- Tracks simulated positions and P&L
- Reports fills through the same WebSocket user data stream interface as live venues
- The OMS, position tracker, and strategies cannot distinguish simulator from real exchange
- **Acceptance:** Full end-to-end paper trading: strategy generates signals, OMS submits to simulator, fills flow back, positions and P&L update correctly

### 7. Testnet Validation
Run full system on Bybit Testnet for at least 2 weeks.
- Simulate adverse conditions: manually cancel orders, send duplicates, disconnect WebSocket mid-order
- Verify all state transitions handled
- Log every order lifecycle event with microsecond timestamps
- Review logs daily for anomalies
- **Acceptance:** Zero memory leaks (verified with AddressSanitizer over 24-hour run)

### 8. Paper Trading Continuous Run
Run the full system against the simulator 24/7 to validate stability.
- Simulator replays recorded data in a loop or generates synthetic market activity
- Monitor P&L, position accuracy, order lifecycle correctness, memory usage
- System must run for 7+ consecutive days without crashes or position divergence
- **Acceptance:** 7-day continuous run with no crashes, no memory leaks, no position divergence

## Training Topics
- Order lifecycle state machines (the most interview-tested component at trading firms)
- Exchange is source of truth for order state, not your system
- Connection pooling and persistent HTTP connections
- Handling race conditions: fill arriving after cancel sent
- Testnet discipline and adverse condition simulation
