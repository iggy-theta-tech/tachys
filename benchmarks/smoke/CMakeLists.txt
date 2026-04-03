add_executable(smoke_bench smoke_bench.cpp)

target_link_libraries(smoke_bench PRIVATE benchmark::benchmark)
target_compile_options(smoke_bench PRIVATE -Wall -Wextra -Wpedantic)
