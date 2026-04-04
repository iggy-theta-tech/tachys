# tachys_core: shared types, platform abstractions, utilities
# Depends on: nothing (base layer)
add_library(tachys_core INTERFACE)

target_include_directories(tachys_core INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
