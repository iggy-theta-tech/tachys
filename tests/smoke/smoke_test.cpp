#include <gtest/gtest.h>
#include <simdjson.h>
#include <format>

TEST(SmokeTest, CppFormat) {
    const std::string s = std::format("hello {}", "world");
    EXPECT_EQ(s, "hello world");
}

TEST(SmokeTest, SimdjsonLinked) {
    EXPECT_GT(simdjson::SIMDJSON_VERSION_MAJOR, 0);
}
