# tachys_infra: async logging, metrics, config
# Depends on: tachys_core
add_library(tachys_infra INTERFACE)

target_link_libraries(tachys_infra INTERFACE tachys_core)
