add_executable(tachys_main main.cpp)

target_link_libraries(tachys_main PRIVATE tachys_core fmt::fmt)

target_compile_options(tachys_main PRIVATE
    -Wall -Wextra -Wpedantic -Werror
)
