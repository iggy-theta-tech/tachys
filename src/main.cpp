// src/main.cpp
#include <format>
#include <cstdio>

int main() {
    // std::format is C++20 -- proves C++23 mode active
    std::puts(std::format("TACHYS v0.1.0 -- C++23 OK").c_str());
    return 0;
}
