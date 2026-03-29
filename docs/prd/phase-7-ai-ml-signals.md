# Phase 7: AI/ML Signal Layer & Agent Orchestration

**Spec Reference:** Section 10 (Pages 23-25)
**Duration:** Weeks 53-64
**Goal:** Add an intelligence layer that generates fair-value estimates and strategic decisions, bridging Python ML skills with the C++ execution engine.

## Architecture Note

The AI layer runs in Python (leveraging the user's current Python expertise) and communicates with C++ via shared memory. This is deliberate: ML development is more productive in Python, and shared memory read overhead (< 100ns) is negligible compared to signal time horizon (seconds to minutes).

## Deliverables

### 1. Shared Memory IPC
High-speed communication channel between Python and C++.
- POSIX shared memory (shm_open + mmap)
- Fixed-layout SignalMessage struct defined in both C++ and Python (ctypes or struct module)
- Python writes, C++ reads
- Seqlock pattern for consistency: atomic sequence counter (writer increments after write, reader checks before/after to detect torn writes)
- SignalBuffer layout (4KB page-aligned): version (uint32_t), sequence (atomic<uint64_t>), num_signals (uint32_t), signals[MAX_SIGNALS]
- Each signal: symbol_id (uint16_t), fair_value (int64_t), confidence (uint16_t 0-10000), model_id (uint16_t), timestamp (uint64_t)
- **Acceptance:** C++ reads Python-written signals with < 100ns latency; torn writes correctly detected and retried

### 2. Order Flow Signal Model
ML model predicting short-term price direction from order book features.
- Features: bid-ask imbalance, order flow imbalance, spread changes, book pressure
- Model: gradient-boosted tree (XGBoost or LightGBM) trained on historical tick data collected since Phase 1
- Export as ONNX for C++ inference, OR keep in Python publishing via shared memory
- **Acceptance:** Model trained on collected data; signal published to shared memory; C++ engine reads and acts on it

### 3. Prediction Market Model
Estimate true probabilities for Polymarket events using external data.
- Category-specific models:
  - Sports: historical data + live game state (ESPN, The Odds API)
  - Politics: polling aggregation, cross-venue prices (Kalshi, Betfair), base rates
  - Crypto events: on-chain data, sentiment
- Publish probability estimates to shared memory
- C++ engine quotes around these fair values
- **Acceptance:** Model produces probability estimates; when model disagrees with market price by > threshold, positions are taken

### 4. Agent Orchestrator
LLM-based agent making high-level strategic decisions.
- Python agent (Claude API or local model)
- Monitors system health and P&L
- Decides which markets to enter/exit based on current conditions
- Adjusts risk parameters based on market regime (low-vol vs high-vol)
- Processes unstructured information (news, social media) for event probabilities
- Publishes config updates (which pairs to trade, risk limits) to C++ via shared memory
- **Acceptance:** Agent makes at least one autonomous config adjustment based on market conditions; change propagates to C++ engine

### 5. AI Signal Paper Trading
Validate AI-driven trading end-to-end using the simulator before live deployment.
- Run the full stack (C++ engine + Python AI layer) against the simulator
- AI models publish fair-value estimates; C++ engine trades against simulator
- Measure simulated P&L from AI-driven positions
- Compare AI signal quality: did positions taken when model disagreed with market result in profit?
- **Acceptance:** AI-driven paper trading runs for 7+ days; P&L and signal accuracy tracked and reported

## Training Topics
- POSIX shared memory and mmap for IPC
- Seqlock pattern: wait-free writer, lock-free reader
- Bridging C++ and Python: ctypes, struct module, memory layout compatibility
- Feature engineering for order flow prediction
- ONNX model deployment in C++
