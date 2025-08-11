# Benchmarking Guide

## Overview
Comprehensive guide for benchmarking userver components and applications using Google Benchmark with coroutine support.

## Google Benchmark Integration

### Setup and Configuration
Link against userver's benchmark helpers in your CMakeLists.txt:
```cmake
target_link_libraries(your-bench-target PRIVATE userver::ubench)
```

Include the necessary headers:
```cpp
#include <benchmark/benchmark.h>
#include <userver/engine/run_standalone.hpp>
```

### Coroutine Environment Benchmarking
Use `engine::RunStandalone` to run benchmarks in a coroutine environment:

```cpp
#include <benchmark/benchmark.h>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/semaphore.hpp>

void semaphore_lock(benchmark::State& state) {
  userver::engine::RunStandalone([&]() {
    std::size_t i = 0;
    userver::engine::Semaphore sem{std::numeric_limits<std::size_t>::max()};
    
    for ([[maybe_unused]] auto _ : state) {
      sem.lock_shared();
      ++i;
    }
    
    for (std::size_t j = 0; j < i; ++j) {
      sem.unlock_shared();
    }
  });
}

BENCHMARK(semaphore_lock);
```

## Benchmarking Best Practices

### Test Setup and Teardown
Proper benchmark structure with setup and teardown:

```cpp
static void BM_DatabaseQuery(benchmark::State& state) {
  // Setup
  auto pool = CreateConnectionPool();
  
  userver::engine::RunStandalone([&]() {
    for (auto _ : state) {
      auto connection = pool->Acquire();
      auto result = connection->Execute("SELECT * FROM users LIMIT 100");
      benchmark::DoNotOptimize(result);
    }
  });
  
  // Teardown happens automatically
}

BENCHMARK(BM_DatabaseQuery)->Threads(4);
```

### Parameterized Benchmarks
Use parameterized benchmarks for testing different configurations:

```cpp
static void BM_CacheLookup(benchmark::State& state) {
  const auto cache_size = state.range(0);
  const auto key_count = state.range(1);
  
  // Setup cache with specified size
  auto cache = CreateCache(cache_size);
  PopulateCache(cache, key_count);
  
  userver::engine::RunStandalone([&]() {
    std::size_t key_index = 0;
    for (auto _ : state) {
      auto result = cache->Get("key_" + std::to_string(key_index % key_count));
      benchmark::DoNotOptimize(result);
      key_index++;
    }
  });
}

BENCHMARK(BM_CacheLookup)
    ->Args({1000, 100})    // cache_size=1000, key_count=100
    ->Args({10000, 1000})  // cache_size=10000, key_count=1000
    ->Args({100000, 10000}); // cache_size=100000, key_count=10000
```

### Multi-threaded Benchmarks
Test concurrent performance with multi-threaded benchmarks:

```cpp
static void BM_ConcurrentHttpCalls(benchmark::State& state) {
  const auto thread_count = state.range(0);
  const auto concurrent_requests = state.range(1);
  
  userver::engine::RunStandalone(thread_count, [&]() {
    for (auto _ : state) {
      std::vector<userver::engine::Task> tasks;
      tasks.reserve(concurrent_requests);
      
      for (int i = 0; i < concurrent_requests; ++i) {
        tasks.push_back(userver::engine::Async("http_call", []() {
          // Simulate HTTP call
          userver::engine::SleepFor(std::chrono::milliseconds(10));
        }));
      }
      
      for (auto& task : tasks) {
        task.Get();
      }
    }
  });
}

BENCHMARK(BM_ConcurrentHttpCalls)->Args({4, 10})->Args({8, 20});
```

## Common Benchmarking Scenarios

### HTTP Handler Performance
Benchmark HTTP handlers under various load conditions:

```cpp
static void BM_HttpHandler(benchmark::State& state) {
  auto handler = CreateTestHandler();
  auto request = CreateTestRequest();
  
  for (auto _ : state) {
    auto response = handler->HandleRequest(request);
    benchmark::DoNotOptimize(response);
  }
}

BENCHMARK(BM_HttpHandler);
```

### Database Operation Performance
Test database operations with different query patterns:

```cpp
static void BM_DatabaseInsert(benchmark::State& state) {
  auto pool = CreateTestPool();
  
  userver::engine::RunStandalone([&]() {
    for (auto _ : state) {
      auto connection = pool->Acquire();
      connection->Execute("INSERT INTO users (name, email) VALUES ($1, $2)", 
                         "Test User", "test@example.com");
    }
  });
}

BENCHMARK(BM_DatabaseInsert);
```

### Cache Performance
Measure cache hit/miss performance and eviction behavior:

```cpp
static void BM_CacheHitRate(benchmark::State& state) {
  const auto cache_size = 1000;
  const auto working_set_size = state.range(0);
  
  auto cache = CreateLruCache(cache_size);
  PopulateCache(cache, working_set_size);
  
  userver::engine::RunStandalone([&]() {
    std::size_t key_index = 0;
    for (auto _ : state) {
      // Alternate between hot and cold keys
      std::string key;
      if (key_index % 10 < 8) {
        // 80% hot keys
        key = "hot_key_" + std::to_string(key_index % (working_set_size / 10));
      } else {
        // 20% cold keys
        key = "cold_key_" + std::to_string(key_index);
      }
      
      auto result = cache->Get(key);
      benchmark::DoNotOptimize(result);
      key_index++;
    }
  });
}

BENCHMARK(BM_CacheHitRate)->Arg(100)->Arg(500)->Arg(1000);
```

## Performance Analysis Tools

### Benchmark Output Analysis
Run benchmarks with different output formats for analysis:

```bash
# Run benchmarks and output to console
./your-benchmark --benchmark_out_format=console

# Run benchmarks and output to JSON for detailed analysis
./your-benchmark --benchmark_out=results.json --benchmark_out_format=json

# Run specific benchmarks
./your-benchmark --benchmark_filter=BM_Database*

# Run with different repetition counts
./your-benchmark --benchmark_repetitions=5
```

### Statistical Analysis
Use benchmark statistics for performance validation:

```cpp
static void BM_JsonParsing(benchmark::State& state) {
  const std::string json_data = GenerateTestData(state.range(0));
  
  for (auto _ : state) {
    auto result = ParseJson(json_data);
    benchmark::DoNotOptimize(result);
  }
  
  state.SetBytesProcessed(state.iterations() * json_data.size());
  state.SetComplexityN(state.range(0));
}

BENCHMARK(BM_JsonParsing)->Range(1<<10, 1<<20)->Complexity();
```

## Benchmarking Anti-Patterns

### Common Mistakes
- Not running benchmarks in coroutine environment for userver components
- Including setup/teardown time in measurements
- Not using `benchmark::DoNotOptimize()` to prevent compiler optimizations
- Running benchmarks with insufficient iterations
- Ignoring warm-up periods
- Not testing with realistic data sizes

### What to Avoid
```cpp
// DON'T: Include setup in benchmark loop
static void BM_BadExample(benchmark::State& state) {
  for (auto _ : state) {
    auto expensive_object = CreateExpensiveObject();  // Setup in loop!
    auto result = expensive_object.DoWork();
    benchmark::DoNotOptimize(result);
  }
}

// DO: Separate setup from benchmark loop
static void BM_GoodExample(benchmark::State& state) {
  auto expensive_object = CreateExpensiveObject();  // Setup outside loop
  for (auto _ : state) {
    auto result = expensive_object.DoWork();
    benchmark::DoNotOptimize(result);
  }
}
```

## Continuous Benchmarking

### CI/Integration
Integrate benchmarks into your CI pipeline:

```yaml
# Example GitHub Actions workflow
benchmark:
  runs-on: ubuntu-latest
  steps:
    - name: Build benchmarks
      run: cmake --build . --target your-benchmark
    - name: Run benchmarks
      run: ./your-benchmark --benchmark_out=results.json --benchmark_out_format=json
    - name: Compare with baseline
      run: python compare_benchmarks.py results.json baseline.json
```

### Performance Regression Detection
Implement performance regression detection:

```python
# Example regression detection script
import json
import sys

def check_regression(new_results, baseline_results, threshold=0.1):
    for benchmark in new_results['benchmarks']:
        name = benchmark['name']
        new_time = benchmark['real_time']
        
        baseline_benchmark = next((b for b in baseline_results['benchmarks'] 
                                 if b['name'] == name), None)
        if baseline_benchmark:
            baseline_time = baseline_benchmark['real_time']
            if new_time > baseline_time * (1 + threshold):
                print(f"Performance regression detected in {name}")
                return False
    return True
```

## Benchmarking Checklist

### Before Running Benchmarks
- [ ] Link against `userver::ubench` target
- [ ] Use `engine::RunStandalone` for coroutine benchmarks
- [ ] Include proper headers (`<benchmark/benchmark.h>`)
- [ ] Use `benchmark::DoNotOptimize()` to prevent optimization
- [ ] Design realistic test scenarios
- [ ] Prepare test data and environment

### During Benchmark Execution
- [ ] Run sufficient iterations for statistical significance
- [ ] Include warm-up periods
- [ ] Test different parameter combinations
- [ ] Run multi-threaded tests when relevant
- [ ] Monitor system resources during tests

### After Benchmark Analysis
- [ ] Analyze results for performance trends
- [ ] Identify performance bottlenecks
- [ ] Compare against baseline metrics
- [ ] Document findings and optimizations
- [ ] Set up continuous benchmarking