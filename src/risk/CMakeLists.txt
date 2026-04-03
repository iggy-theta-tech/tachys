# tachys_risk: pre-trade risk checks, kill switch
# Depends on: tachys_core, tachys_oms
add_library(tachys_risk INTERFACE)

target_link_libraries(tachys_risk INTERFACE tachys_core tachys_oms)
