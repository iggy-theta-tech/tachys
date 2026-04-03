# tachys_feed: WebSocket client, JSON parsing, market data normalization
# Depends on: tachys_core, tachys_infra
add_library(tachys_feed INTERFACE)

target_link_libraries(tachys_feed INTERFACE tachys_core tachys_infra)
