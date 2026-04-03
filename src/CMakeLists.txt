add_executable(tachys_main main.cpp)

target_compile_options(tachys_main PRIVATE
    -Wall -Wextra -Wpedantic -Werror
)
