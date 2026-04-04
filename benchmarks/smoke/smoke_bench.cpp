#include <benchmark/benchmark.h>
#include <format>

static void BM_StdFormat(benchmark::State& state) {
    for (auto _ : state) {
        std::string s = std::format("hello {}", 42);
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_StdFormat);

BENCHMARK_MAIN();
