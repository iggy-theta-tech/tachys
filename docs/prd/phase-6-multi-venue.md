# Phase 6: Multi-Venue Expansion & Polymarket Integration

**Spec Reference:** Section 9 (Pages 22-23)
**Duration:** Weeks 43-52
**Goal:** Connect to multiple exchanges and prediction markets, normalize data across venues, and enable cross-venue strategies.

## Deliverables

### 1. Venue Abstraction Layer
Uniform interface across all connected venues so strategies are venue-agnostic.
- VenueTraits<V> template defining per-venue types: message format, order ID type, fee structure, rate limits
- Each venue specialization (Binance, Bybit, Polymarket) implements the traits
- Strategy layer only interacts with normalized types; never knows which venue it's trading on
- **Acceptance:** Adding a new venue is a 2-day task: implement VenueTraits specialization only

### 2. Multi-Feed Aggregator
Consume and synchronize market data from multiple venues simultaneously.
- Each venue runs its own I/O thread (pinned to separate cores)
- All feeds publish to shared MPSC merge queue
- Strategy thread consumes from merged stream
- Timestamps normalized to common clock
- VenueBook struct holds one order book per venue
- CompositeBook provides best-bid/best-ask across all venues
- **Acceptance:** Books from multiple venues updating simultaneously with correct timestamps

### 3. Polymarket Integration
Connect to Polymarket's CLOB API and trade binary outcome contracts.
- Venue specialization: parse order book, submit limit orders, handle fills
- Binary contracts: YES/NO shares priced 0.00-1.00
- REST + WebSocket API (similar to crypto exchanges)
- Wallet integration for settlement (Polygon, USDC collateral)
- Trading is off-chain via CLOB; settlement is on-chain
- **Acceptance:** Connect to Polymarket, receive book updates, submit and fill test orders

### 4. Cross-Venue Execution
Execute arbitrage legs across two venues simultaneously.
- When arbitrage detector signals: simultaneously submit sell to venue A, buy to venue B
- Non-blocking order submission (fire both, then await acks)
- Partial-fill handling: if only one leg fills, immediately attempt to flatten
- **Acceptance:** Cross-venue arb execution tested on testnet; partial-fill recovery verified

### 5. Fee Normalization
Accurately model fee structures across venues.
- FeeModel<Venue> computes expected fee for a given order
- Maker/taker percentages, tiered by volume, token-based discounts
- All arb calculations subtract worst-case fees from both legs before signaling
- **Acceptance:** Fee calculations verified against actual exchange fee reports

### 6. Multi-Venue Simulator
Extend the simulator to support multiple simultaneous simulated venues.
- Run `ExchangeFeed<Simulator>` and `ExchangeGateway<Simulator>` with different venue configs simultaneously
- Simulate cross-venue arbitrage scenarios with configurable price discrepancies
- Test cross-venue execution (both legs filling, one leg failing, partial fills)
- Enable full multi-venue paper trading before connecting to real secondary exchanges
- **Acceptance:** Cross-venue arb execution works end-to-end in simulator; partial-fill recovery verified in simulated environment

## Training Topics
- Template specialization and traits pattern for venue abstraction
- MPSC queue design for multi-producer scenarios
- Clock synchronization across venues (NTP)
- Polymarket: prediction market mechanics, binary contracts, on-chain settlement
- Leg risk in cross-exchange arbitrage
