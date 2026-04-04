# tachys_book: order book engine, price levels
# Depends on: tachys_core
add_library(tachys_book INTERFACE)

target_link_libraries(tachys_book INTERFACE tachys_core)
