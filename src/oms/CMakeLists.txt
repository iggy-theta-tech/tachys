# tachys_oms: order management, execution, fill reconciliation
# Depends on: tachys_core, tachys_infra
add_library(tachys_oms INTERFACE)

target_link_libraries(tachys_oms INTERFACE tachys_core tachys_infra)
