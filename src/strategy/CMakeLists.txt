# tachys_strategy: market-making and arbitrage signal logic
# Depends on: tachys_core, tachys_book
add_library(tachys_strategy INTERFACE)

target_link_libraries(tachys_strategy INTERFACE tachys_core tachys_book)
