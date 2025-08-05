# Performance Analysis

## Overview

Comprehensive performance debugging and profiling strategies for userver applications, covering CPU profiling, memory analysis, I/O bottleneck detection, and context switch optimization.

## Performance Profiling Framework

### Context Switch Profiling

**Configuration Setup**
```yaml
# Static config for context switch profiling
task_processors:
  main-task-processor:
    worker_threads: 4
    task-trace:
      every: 1                        # Profile every task
      max-context-switch-count: 1000  # Limit trace size
      logger: tracer                  # Separate logger

components:
  logging:
    fs-task-processor: fs-task-processor
    loggers:
      tracer:
        file_path: /var/log/service/tracer.log
        level: info  # Set to debug for stacktraces
        overflow_behavior: discard
```

**Analysis Techniques**
```cpp
// Enable context switch monitoring
class PerformanceAnalyzer {
public:
    void AnalyzeContextSwitches() {
        // Look for suspicious patterns:
        // 1. Long delays between state changes
        // 2. Frequent task switching
        // 3. Tasks stuck in kSuspended state
        
        LOG_INFO() << "Task state analysis:"
                   << " task_id=" << GetTaskId()
                   << " state_change_delay=" << delay.count() << "us";
    }
    
    void DetectPerformanceIssues() {
        // Pattern 1: Task waiting too long
        if (delay > std::chrono::milliseconds(100)) {
            LOG_WARNING() << "Slow task transition detected"
                         << logging::LogExtra::Stacktrace();
        }
        
        // Pattern 2: Excessive context switching
        if (context_switches_per_second > 10000) {
            LOG_ERROR() << "Context switch storm detected";
        }
    }
};
```

### CPU Profiling

**Profiling Integration**
```cpp
// CPU-intensive operation profiling
class CPUProfiler {
public:
    template<typename Func>
    auto ProfileCPUIntensive(const std::string& operation_name, Func&& func) {
        tracing::Span span(operation_name);
        auto start = std::chrono::steady_clock::now();
        
        auto result = std::forward<Func>(func)();
        
        auto duration = std::chrono::steady_clock::now() - start;
        span.AddTag("cpu_time_ms", 
                   std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
        
        if (duration > std::chrono::milliseconds(100)) {
            LOG_WARNING() << "CPU-intensive operation detected: " << operation_name
                         << " duration=" << duration.count() << "ms";
        }
        
        return result;
    }
};

// Usage example
void OptimizeHotPath() {
    CPUProfiler profiler;
    
    auto result = profiler.ProfileCPUIntensive("data_processing", [&]() {
        return ProcessLargeDataset(dataset);
    });
}
```

**Hot Path Identification**
```cpp
// Identify performance bottlenecks
class HotPathAnalyzer {
private:
    std::unordered_map<std::string, PerformanceMetrics> metrics_;
    
public:
    void RecordOperation(const std::string& operation, 
                        std::chrono::nanoseconds duration) {
        auto& metric = metrics_[operation];
        metric.total_time += duration;
        metric.call_count++;
        metric.max_time = std::max(metric.max_time, duration);
        
        // Detect hot paths
        if (metric.call_count % 1000 == 0) {
            auto avg_time = metric.total_time / metric.call_count;
            if (avg_time > std::chrono::milliseconds(10)) {
                LOG_WARNING() << "Hot path detected: " << operation
                             << " avg_time=" << avg_time.count() << "ns"
                             << " call_count=" << metric.call_count;
            }
        }
    }
    
    void GenerateReport() {
        LOG_INFO() << "Performance Report:";
        for (const auto& [operation, metric] : metrics_) {
            auto avg_time = metric.total_time / metric.call_count;
            LOG_INFO() << "Operation: " << operation
                      << " calls=" << metric.call_count
                      << " avg=" << avg_time.count() << "ns"
                      << " max=" << metric.max_time.count() << "ns";
        }
    }
};
```

### Memory Profiling

**Memory Usage Analysis**
```cpp
// Memory profiling utilities
class MemoryProfiler {
public:
    struct MemorySnapshot {
        size_t heap_size;
        size_t stack_size;
        size_t virtual_memory;
        std::chrono::steady_clock::time_point timestamp;
    };
    
    MemorySnapshot TakeSnapshot() {
        // Get memory statistics
        return MemorySnapshot{
            .heap_size = GetHeapSize(),
            .stack_size = GetStackSize(),
            .virtual_memory = GetVirtualMemorySize(),
            .timestamp = std::chrono::steady_clock::now()
        };
    }
    
    void DetectMemoryLeaks() {
        static auto baseline = TakeSnapshot();
        auto current = TakeSnapshot();
        
        auto heap_growth = current.heap_size - baseline.heap_size;
        auto time_diff = current.timestamp - baseline.timestamp;
        
        if (heap_growth > 100 * 1024 * 1024) { // 100MB growth
            LOG_ERROR() << "Potential memory leak detected"
                       << " heap_growth=" << heap_growth
                       << " time_elapsed=" << time_diff.count() << "ns"
                       << logging::LogExtra::Stacktrace();
        }
    }
    
    void ProfileLargeAllocations() {
        // Hook into allocation tracking
        LOG_DEBUG() << "Large allocation detected"
                   << " size=" << allocation_size
                   << logging::LogExtra::Stacktrace();
    }
};
```

**Memory Leak Detection**
```cpp
// RAII-based memory tracking
template<typename T>
class TrackedResource {
private:
    std::unique_ptr<T> resource_;
    std::string resource_name_;
    std::chrono::steady_clock::time_point created_at_;
    
public:
    TrackedResource(std::unique_ptr<T> resource, std::string name)
        : resource_(std::move(resource))
        , resource_name_(std::move(name))
        , created_at_(std::chrono::steady_clock::now()) {
        
        LOG_DEBUG() << "Resource created: " << resource_name_;
    }
    
    ~TrackedResource() {
        auto lifetime = std::chrono::steady_clock::now() - created_at_;
        LOG_DEBUG() << "Resource destroyed: " << resource_name_
                   << " lifetime=" << lifetime.count() << "ns";
        
        if (lifetime > std::chrono::hours(1)) {
            LOG_WARNING() << "Long-lived resource detected: " << resource_name_;
        }
    }
    
    T* get() const { return resource_.get(); }
    T& operator*() const { return *resource_; }
    T* operator->() const { return resource_.get(); }
};
```

## I/O Performance Analysis

### Database Query Profiling

**Query Performance Monitoring**
```cpp
// Database operation profiling
class DatabaseProfiler {
public:
    template<typename QueryFunc>
    auto ProfileQuery(const std::string& query_name, QueryFunc&& func) {
        tracing::Span span("db_query");
        span.AddTag("query_name", query_name);
        
        auto start = std::chrono::steady_clock::now();
        
        try {
            auto result = std::forward<QueryFunc>(func)();
            
            auto duration = std::chrono::steady_clock::now() - start;
            span.AddTag("duration_ms", 
                       std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
            span.AddTag("status", "success");
            
            // Detect slow queries
            if (duration > std::chrono::milliseconds(1000)) {
                LOG_WARNING() << "Slow query detected: " << query_name
                             << " duration=" << duration.count() << "ms";
            }
            
            return result;
        } catch (const std::exception& e) {
            span.AddTag("status", "error");
            span.AddTag("error", e.what());
            throw;
        }
    }
};

// Usage with PostgreSQL
void OptimizeDatabaseQueries() {
    DatabaseProfiler profiler;
    
    auto result = profiler.ProfileQuery("user_lookup", [&]() {
        return pg_cluster_->Execute(
            storages::postgres::ClusterHostType::kSlave,
            "SELECT * FROM users WHERE id = $1",
            user_id
        );
    });
}
```

### HTTP Client Performance

**HTTP Request Profiling**
```cpp
// HTTP client performance monitoring
class HttpClientProfiler {
public:
    void ProfileHttpRequest(const clients::http::Request& request) {
        tracing::Span span("http_client_request");
        span.AddTag("method", request.method());
        span.AddTag("url", request.url());
        
        auto start = std::chrono::steady_clock::now();
        
        try {
            auto response = request.perform();
            
            auto duration = std::chrono::steady_clock::now() - start;
            span.AddTag("duration_ms", 
                       std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
            span.AddTag("status_code", response->status_code());
            
            // Analyze response times
            if (duration > std::chrono::seconds(5)) {
                LOG_ERROR() << "HTTP request timeout risk"
                           << " url=" << request.url()
                           << " duration=" << duration.count() << "ms";
            }
            
        } catch (const std::exception& e) {
            span.AddTag("error", e.what());
            LOG_ERROR() << "HTTP request failed: " << e.what();
            throw;
        }
    }
};
```

## Advanced Profiling Techniques

### Flame Graph Generation

**Performance Data Collection**
```cpp
// Collect performance data for flame graphs
class FlameGraphCollector {
private:
    struct CallStackEntry {
        std::string function_name;
        std::chrono::nanoseconds duration;
        std::vector<CallStackEntry> children;
    };
    
    thread_local std::stack<CallStackEntry*> call_stack_;
    
public:
    class ScopedProfiler {
    private:
        FlameGraphCollector& collector_;
        std::chrono::steady_clock::time_point start_time_;
        std::string function_name_;
        
    public:
        ScopedProfiler(FlameGraphCollector& collector, std::string name)
            : collector_(collector)
            , start_time_(std::chrono::steady_clock::now())
            , function_name_(std::move(name)) {
            collector_.EnterFunction(function_name_);
        }
        
        ~ScopedProfiler() {
            auto duration = std::chrono::steady_clock::now() - start_time_;
            collector_.ExitFunction(function_name_, duration);
        }
    };
    
    void EnterFunction(const std::string& name);
    void ExitFunction(const std::string& name, std::chrono::nanoseconds duration);
    void GenerateFlameGraph();
};

#define PROFILE_FUNCTION() \
    FlameGraphCollector::ScopedProfiler _prof(flame_graph_collector, __FUNCTION__)
```

### Benchmark Integration

**Performance Regression Detection**
```cpp
// Benchmark-based performance monitoring
class PerformanceBenchmark {
public:
    struct BenchmarkResult {
        std::chrono::nanoseconds min_time;
        std::chrono::nanoseconds max_time;
        std::chrono::nanoseconds avg_time;
        size_t iterations;
    };
    
    template<typename Func>
    BenchmarkResult RunBenchmark(const std::string& name, Func&& func, 
                                size_t iterations = 1000) {
        std::vector<std::chrono::nanoseconds> times;
        times.reserve(iterations);
        
        for (size_t i = 0; i < iterations; ++i) {
            auto start = std::chrono::steady_clock::now();
            std::forward<Func>(func)();
            auto end = std::chrono::steady_clock::now();
            times.push_back(end - start);
        }
        
        auto min_time = *std::min_element(times.begin(), times.end());
        auto max_time = *std::max_element(times.begin(), times.end());
        auto total_time = std::accumulate(times.begin(), times.end(), 
                                        std::chrono::nanoseconds{0});
        auto avg_time = total_time / iterations;
        
        BenchmarkResult result{min_time, max_time, avg_time, iterations};
        
        LOG_INFO() << "Benchmark: " << name
                  << " min=" << min_time.count() << "ns"
                  << " max=" << max_time.count() << "ns"
                  << " avg=" << avg_time.count() << "ns"
                  << " iterations=" << iterations;
        
        return result;
    }
    
    void DetectRegressions(const std::string& name, 
                          const BenchmarkResult& current,
                          const BenchmarkResult& baseline) {
        auto regression_threshold = 1.2; // 20% slower
        
        if (current.avg_time > baseline.avg_time * regression_threshold) {
            LOG_ERROR() << "Performance regression detected: " << name
                       << " baseline=" << baseline.avg_time.count() << "ns"
                       << " current=" << current.avg_time.count() << "ns"
                       << " regression=" << 
                          (static_cast<double>(current.avg_time.count()) / 
                           baseline.avg_time.count() - 1.0) * 100 << "%";
        }
    }
};
```

## Performance Optimization Patterns

### Async Operation Optimization

**Coroutine Performance**
```cpp
// Optimize async operations
class AsyncOptimizer {
public:
    // Pattern 1: Batch operations
    engine::TaskWithResult<std::vector<Result>> 
    BatchProcess(const std::vector<Input>& inputs) {
        std::vector<engine::TaskWithResult<Result>> tasks;
        tasks.reserve(inputs.size());
        
        // Launch all tasks concurrently
        for (const auto& input : inputs) {
            tasks.emplace_back(ProcessSingleAsync(input));
        }
        
        // Wait for all results
        std::vector<Result> results;
        results.reserve(inputs.size());
        
        for (auto& task : tasks) {
            results.emplace_back(co_await task);
        }
        
        co_return results;
    }
    
    // Pattern 2: Pipeline processing
    engine::TaskWithResult<void> PipelineProcess() {
        auto producer = ProduceDataAsync();
        auto consumer = ConsumeDataAsync();
        
        // Run producer and consumer concurrently
        co_await engine::WaitAllChecked(std::move(producer), std::move(consumer));
    }
};
```

### Cache Optimization

**Performance-Aware Caching**
```cpp
// Cache performance monitoring
class PerformanceCache {
private:
    struct CacheMetrics {
        std::atomic<size_t> hits{0};
        std::atomic<size_t> misses{0};
        std::atomic<size_t> evictions{0};
        std::chrono::nanoseconds total_lookup_time{0};
    };
    
    CacheMetrics metrics_;
    
public:
    template<typename Key, typename Value>
    std::optional<Value> Get(const Key& key) {
        auto start = std::chrono::steady_clock::now();
        
        auto result = cache_.Get(key);
        
        auto lookup_time = std::chrono::steady_clock::now() - start;
        metrics_.total_lookup_time += lookup_time;
        
        if (result) {
            metrics_.hits++;
        } else {
            metrics_.misses++;
        }
        
        // Monitor cache performance
        if ((metrics_.hits + metrics_.misses) % 10000 == 0) {
            ReportCacheMetrics();
        }
        
        return result;
    }
    
    void ReportCacheMetrics() {
        auto total_requests = metrics_.hits + metrics_.misses;
        auto hit_rate = static_cast<double>(metrics_.hits) / total_requests;
        auto avg_lookup_time = metrics_.total_lookup_time / total_requests;
        
        LOG_INFO() << "Cache performance:"
                  << " hit_rate=" << hit_rate * 100 << "%"
                  << " avg_lookup_time=" << avg_lookup_time.count() << "ns"
                  << " evictions=" << metrics_.evictions;
        
        if (hit_rate < 0.8) {
            LOG_WARNING() << "Low cache hit rate detected";
        }
    }
};
```

## Integration Points

**Cross-References**
- [`troubleshooting-workflows.md`](./troubleshooting-workflows.md) - General debugging workflows
- [`profiling-techniques.md`](./profiling-techniques.md) - Advanced profiling methods
- [`monitoring-debug.md`](./monitoring-debug.md) - Performance monitoring setup

**Memory Bank References**
- [`performance-research.md`](../../memory-bank/research/performance-research.md) - Performance research findings
- [`async-programming.md`](../../memory-bank/main/async-programming.md) - Async performance patterns
- [`advanced-monitoring`](../../memory-bank/specialized/advanced-monitoring/) - Monitoring integration

## Best Practices

### Performance Testing
- Establish performance baselines
- Regular regression testing
- Load testing with realistic data
- Monitor key performance indicators

### Optimization Strategy
- Profile before optimizing
- Focus on hot paths first
- Measure optimization impact
- Document performance characteristics

### Production Monitoring
- Continuous performance monitoring
- Automated alerting on regressions
- Regular performance reviews
- Capacity planning based on metrics