// src/main.cpp
#include <fmt/core.h>
#include <format>

int main() {
    fmt::print("TACHYS v0.1.0 -- C++23 OK\n");
    fmt::print("fmt {}.{}.{}\n",
        FMT_VERSION / 10000,
        (FMT_VERSION % 10000) / 100,
        FMT_VERSION % 100);
    return 0;
}
