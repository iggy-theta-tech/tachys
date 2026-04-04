add_executable(smoke_test smoke_test.cpp)

target_link_libraries(smoke_test PRIVATE GTest::gtest_main simdjson::simdjson)
target_compile_options(smoke_test PRIVATE -Wall -Wextra -Wpedantic)

include(GoogleTest)
gtest_discover_tests(smoke_test)
