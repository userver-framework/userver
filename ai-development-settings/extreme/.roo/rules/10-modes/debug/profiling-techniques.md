# Profiling Techniques

## Overview

Advanced profiling strategies for userver applications, covering memory profiling, CPU profiling, I/O analysis, and specialized debugging tools integration.

## Memory Profiling Strategies

### Production Memory Profiling

**Runtime Memory Profiling Setup**
```yaml
# Static configuration for memory profiling
components_manager:
  components:
    server:
      listener:
        port: 8080
        task_processor: main-task-processor
    
    # Enable memory profiling endpoint
    handler-memory-profile:
      path: /service/memory-profile
      method: GET
      task_processor: main-task-processor
      
    # Configure profiling options
    manager-controller:
      coro_pool:
        initial_size: 1000
        max_size: 16384
        stack_size: 256KB
        # Enable stack usage monitoring
        stack_usage_monitor_enabled: true
```

**Memory Leak Detection**
```cpp
// Advanced memory tracking
class MemoryProfiler {
private:
    struct AllocationInfo {
        size_t size;
        std::string stack_trace;
        std::chrono::steady_clock::time_point timestamp;
        std::thread::id thread_id;
    };
    
    std::unordered_map<void*, AllocationInfo> allocations_;
    std::mutex allocations_mutex_;
    std::atomic<size_t> total_allocated_{0};
    std::atomic<size_t> peak_memory_{0};
    
public:
    void RecordAllocation(void* ptr, size_t size) {
        if (!ptr) return;
        
        std::lock_guard lock(allocations_mutex_);
        
        AllocationInfo info{
            .size = size,
            .stack_trace = GetCurrentStackTrace(),
            .timestamp = std::chrono::steady_clock::now(),
            .thread_id = std::this_thread::get_id()
        };
        
        allocations_[ptr] = std::move(info);
        
        auto current_total = total_allocated_.fetch_add(size) + size;
        
        // Update peak memory usage
        size_t expected_peak = peak_memory_.load();
        while (current_total > expected_peak && 
               !peak_memory_.compare_exchange_weak(expected_peak, current_total)) {
            // Retry if another thread updated peak_memory_
        }
        
        // Alert on large allocations
        if (size > 10 * 1024 * 1024) { // 10MB
            LOG_WARNING() << "Large allocation detected: " << size << " bytes"
                         << logging::LogExtra::Stacktrace();
        }
    }
    
    void RecordDeallocation(void* ptr) {
        if (!ptr) return;
        
        std::lock_guard lock(allocations_mutex_);
        
        auto it = allocations_.find(ptr);
        if (it != allocations_.end()) {
            total_allocated_.fetch_sub(it->second.size);
            allocations_.erase(it);
        }
    }
    
    void DetectLeaks() {
        std::lock_guard lock(allocations_mutex_);
        
        auto now = std::chrono::steady_clock::now();
        size_t leaked_memory = 0;
        size_t long_lived_allocations = 0;
        
        for (const auto& [ptr, info] : allocations_) {
            auto age = now - info.timestamp;
            
            // Consider allocations older than 1 hour as potential leaks
            if (age > std::chrono::hours(1)) {
                leaked_memory += info.size;
                long_lived_allocations++;
                
                LOG_WARNING() << "Potential memory leak detected:"
                             << " ptr=" << ptr
                             << " size=" << info.size
                             << " age=" << std::chrono::duration_cast<std::chrono::minutes>(age).count() << "min"
                             << " thread=" << info.thread_id
                             << " stack_trace=" << info.stack_trace;
            }
        }
        
        if (leaked_memory > 100 * 1024 * 1024) { // 100MB
            LOG_ERROR() << "Significant memory leak detected:"
                       << " leaked_memory=" << leaked_memory
                       << " long_lived_allocations=" << long_lived_allocations;
        }
    }
    
    void GenerateMemoryReport() {
        std::lock_guard lock(allocations_mutex_);
        
        LOG_INFO() << "Memory Profile Report:"
                  << " current_allocations=" << allocations_.size()
                  << " total_allocated=" << total_allocated_.load()
                  << " peak_memory=" << peak_memory_.load();
        
        // Group allocations by stack trace
        std::unordered_map<std::string, std::pair<size_t, size_t>> stack_groups;
        
        for (const auto& [ptr, info] : allocations_) {
            auto& group = stack_groups[info.stack_trace];
            group.first += info.size;  // Total size
            group.second++;           // Count
        }
        
        // Report top allocation sites
        std::vector<std::pair<std::string, std::pair<size_t, size_t>>> sorted_groups(
            stack_groups.begin(), stack_groups.end());
        
        std::sort(sorted_groups.begin(), sorted_groups.end(),
                 [](const auto& a, const auto& b) {
                     return a.second.first > b.second.first;
                 });
        
        LOG_INFO() << "Top allocation sites:";
        for (size_t i = 0; i < std::min(sorted_groups.size(), size_t{10}); ++i) {
            const auto& [stack_trace, size_count] = sorted_groups[i];
            LOG_INFO() << "Site " << i + 1 << ":"
                      << " total_size=" << size_count.first
                      << " count=" << size_count.second
                      << " avg_size=" << (size_count.first / size_count.second)
                      << " stack=" << stack_trace;
        }
    }
    
private:
    std::string GetCurrentStackTrace() {
        // Implementation depends on platform
        // Use logging::LogExtra::Stacktrace() or similar
        return "stack_trace_placeholder";
    }
};
```

### Heap Profiling Integration

**Heap Analysis Tools**
```cpp
// Heap profiler integration
class HeapProfiler {
public:
    struct HeapSnapshot {
        size_t total_heap_size;
        size_t used_heap_size;
        size_t free_heap_size;
        size_t largest_free_block;
        size_t fragmentation_ratio;
        std::chrono::steady_clock::time_point timestamp;
    };
    
    HeapSnapshot TakeHeapSnapshot() {
        HeapSnapshot snapshot;
        snapshot.timestamp = std::chrono::steady_clock::now();
        
        // Get heap statistics (implementation specific)
        auto heap_info = GetHeapInfo();
        
        snapshot.total_heap_size = heap_info.total_size;
        snapshot.used_heap_size = heap_info.used_size;
        snapshot.free_heap_size = heap_info.free_size;
        snapshot.largest_free_block = heap_info.largest_free;
        
        // Calculate fragmentation ratio
        if (snapshot.free_heap_size > 0) {
            snapshot.fragmentation_ratio = 
                (snapshot.free_heap_size - snapshot.largest_free_block) * 100 / 
                snapshot.free_heap_size;
        }
        
        return snapshot;
    }
    
    void AnalyzeHeapFragmentation() {
        auto snapshot = TakeHeapSnapshot();
        
        LOG_INFO() << "Heap Analysis:"
                  << " total=" << snapshot.total_heap_size
                  << " used=" << snapshot.used_heap_size
                  << " free=" << snapshot.free_heap_size
                  << " largest_free=" << snapshot.largest_free_block
                  << " fragmentation=" << snapshot.fragmentation_ratio << "%";
        
        // Alert on high fragmentation
        if (snapshot.fragmentation_ratio > 50) {
            LOG_WARNING() << "High heap fragmentation detected: " 
                         << snapshot.fragmentation_ratio << "%";
        }
        
        // Alert on low free memory
        double free_ratio = static_cast<double>(snapshot.free_heap_size) / 
                           snapshot.total_heap_size * 100;
        if (free_ratio < 10) {
            LOG_ERROR() << "Low free heap memory: " << free_ratio << "%";
        }
    }
    
private:
    struct HeapInfo {
        size_t total_size;
        size_t used_size;
        size_t free_size;
        size_t largest_free;
    };
    
    HeapInfo GetHeapInfo() {
        // Platform-specific heap information gathering
        // This is a placeholder implementation
        return HeapInfo{};
    }
};
```

## CPU Profiling Techniques

### Context Switch Profiling

**Advanced Context Switch Analysis**
```cpp
// Context switch profiler based on userver documentation
class ContextSwitchProfiler {
private:
    struct TaskStateTransition {
        std::string task_id;
        std::string from_state;
        std::string to_state;
        std::chrono::nanoseconds delay;
        std::string stack_trace;
        std::chrono::steady_clock::time_point timestamp;
    };
    
    std::vector<TaskStateTransition> transitions_;
    std::mutex transitions_mutex_;
    
public:
    void RecordStateTransition(const std::string& task_id,
                              const std::string& from_state,
                              const std::string& to_state,
                              std::chrono::nanoseconds delay) {
        std::lock_guard lock(transitions_mutex_);
        
        TaskStateTransition transition{
            .task_id = task_id,
            .from_state = from_state,
            .to_state = to_state,
            .delay = delay,
            .stack_trace = GetStackTraceIfNeeded(delay),
            .timestamp = std::chrono::steady_clock::now()
        };
        
        transitions_.push_back(std::move(transition));
        
        // Analyze suspicious transitions
        AnalyzeTransition(transitions_.back());
        
        // Limit history size
        if (transitions_.size() > 10000) {
            transitions_.erase(transitions_.begin(), 
                             transitions_.begin() + 1000);
        }
    }
    
    void AnalyzeContextSwitchPatterns() {
        std::lock_guard lock(transitions_mutex_);
        
        // Pattern 1: Long delays in state transitions
        std::vector<TaskStateTransition> slow_transitions;
        for (const auto& transition : transitions_) {
            if (transition.delay > std::chrono::milliseconds(100)) {
                slow_transitions.push_back(transition);
            }
        }
        
        if (!slow_transitions.empty()) {
            LOG_WARNING() << "Slow state transitions detected: " << slow_transitions.size();
            
            // Group by transition type
            std::unordered_map<std::string, std::vector<std::chrono::nanoseconds>> transition_delays;
            for (const auto& transition : slow_transitions) {
                std::string key = transition.from_state + "->" + transition.to_state;
                transition_delays[key].push_back(transition.delay);
            }
            
            // Report statistics
            for (const auto& [transition_type, delays] : transition_delays) {
                auto avg_delay = std::accumulate(delays.begin(), delays.end(), 
                                               std::chrono::nanoseconds{0}) / delays.size();
                auto max_delay = *std::max_element(delays.begin(), delays.end());
                
                LOG_WARNING() << "Transition: " << transition_type
                             << " count=" << delays.size()
                             << " avg_delay=" << avg_delay.count() << "ns"
                             << " max_delay=" << max_delay.count() << "ns";
            }
        }
        
        // Pattern 2: Frequent task switching
        std::unordered_map<std::string, size_t> task_switch_counts;
        for (const auto& transition : transitions_) {
            task_switch_counts[transition.task_id]++;
        }
        
        for (const auto& [task_id, count] : task_switch_counts) {
            if (count > 1000) { // High switch frequency
                LOG_WARNING() << "High context switch frequency: task=" << task_id
                             << " switches=" << count;
            }
        }
    }
    
private:
    void AnalyzeTransition(const TaskStateTransition& transition) {
        // Detect problematic patterns
        if (transition.from_state == "kSuspended" && 
            transition.to_state == "kQueued" &&
            transition.delay > std::chrono::milliseconds(1000)) {
            
            LOG_ERROR() << "Task stuck in suspended state: " << transition.task_id
                       << " delay=" << transition.delay.count() << "ns"
                       << " stack_trace=" << transition.stack_trace;
        }
        
        if (transition.delay > std::chrono::seconds(1)) {
            LOG_ERROR() << "Extremely slow state transition detected:"
                       << " task=" << transition.task_id
                       << " transition=" << transition.from_state << "->" << transition.to_state
                       << " delay=" << transition.delay.count() << "ns";
        }
    }
    
    std::string GetStackTraceIfNeeded(std::chrono::nanoseconds delay) {
        // Only capture stack trace for slow transitions to reduce overhead
        if (delay > std::chrono::milliseconds(100)) {
            return "stack_trace_for_slow_transition";
        }
        return "";
    }
};
```

### CPU Hotspot Detection

**CPU Profiling Integration**
```cpp
// CPU hotspot detector
class CPUHotspotDetector {
private:
    struct FunctionProfile {
        std::string function_name;
        std::atomic<size_t> call_count{0};
        std::atomic<std::chrono::nanoseconds::rep> total_time{0};
        std::atomic<std::chrono::nanoseconds::rep> max_time{0};
        std::chrono::steady_clock::time_point first_call;
        std::chrono::steady_clock::time_point last_call;
    };
    
    std::unordered_map<std::string, std::unique_ptr<FunctionProfile>> profiles_;
    std::shared_mutex profiles_mutex_;
    
public:
    class ScopedProfiler {
    private:
        CPUHotspotDetector& detector_;
        std::string function_name_;
        std::chrono::steady_clock::time_point start_time_;
        
    public:
        ScopedProfiler(CPUHotspotDetector& detector, std::string function_name)
            : detector_(detector)
            , function_name_(std::move(function_name))
            , start_time_(std::chrono::steady_clock::now()) {}
        
        ~ScopedProfiler() {
            auto end_time = std::chrono::steady_clock::now();
            auto duration = end_time - start_time_;
            detector_.RecordFunctionCall(function_name_, duration);
        }
    };
    
    void RecordFunctionCall(const std::string& function_name, 
                           std::chrono::nanoseconds duration) {
        FunctionProfile* profile = GetOrCreateProfile(function_name);
        
        profile->call_count.fetch_add(1);
        profile->total_time.fetch_add(duration.count());
        profile->last_call = std::chrono::steady_clock::now();
        
        // Update max time atomically
        auto current_max = profile->max_time.load();
        while (duration.count() > current_max && 
               !profile->max_time.compare_exchange_weak(current_max, duration.count())) {
            // Retry if another thread updated max_time
        }
        
        // Alert on slow function calls
        if (duration > std::chrono::milliseconds(100)) {
            LOG_WARNING() << "Slow function call detected: " << function_name
                         << " duration=" << duration.count() << "ns";
        }
    }
    
    void GenerateHotspotReport() {
        std::shared_lock lock(profiles_mutex_);
        
        struct ProfileStats {
            std::string name;
            size_t call_count;
            std::chrono::nanoseconds total_time;
            std::chrono::nanoseconds avg_time;
            std::chrono::nanoseconds max_time;
            double cpu_percentage;
        };
        
        std::vector<ProfileStats> stats;
        std::chrono::nanoseconds total_cpu_time{0};
        
        // Collect statistics
        for (const auto& [name, profile] : profiles_) {
            auto call_count = profile->call_count.load();
            auto total_time = std::chrono::nanoseconds{profile->total_time.load()};
            auto max_time = std::chrono::nanoseconds{profile->max_time.load()};
            
            if (call_count > 0) {
                auto avg_time = total_time / call_count;
                total_cpu_time += total_time;
                
                stats.push_back(ProfileStats{
                    .name = name,
                    .call_count = call_count,
                    .total_time = total_time,
                    .avg_time = avg_time,
                    .max_time = max_time,
                    .cpu_percentage = 0.0 // Will be calculated later
                });
            }
        }
        
        // Calculate CPU percentages
        for (auto& stat : stats) {
            stat.cpu_percentage = static_cast<double>(stat.total_time.count()) / 
                                 total_cpu_time.count() * 100.0;
        }
        
        // Sort by total CPU time
        std::sort(stats.begin(), stats.end(),
                 [](const auto& a, const auto& b) {
                     return a.total_time > b.total_time;
                 });
        
        // Report top CPU consumers
        LOG_INFO() << "CPU Hotspot Report (Top 10):";
        for (size_t i = 0; i < std::min(stats.size(), size_t{10}); ++i) {
            const auto& stat = stats[i];
            LOG_INFO() << "Function: " << stat.name
                      << " calls=" << stat.call_count
                      << " total_time=" << stat.total_time.count() << "ns"
                      << " avg_time=" << stat.avg_time.count() << "ns"
                      << " max_time=" << stat.max_time.count() << "ns"
                      << " cpu_pct=" << std::fixed << std::setprecision(2) << stat.cpu_percentage << "%";
        }
        
        // Identify hotspots
        for (const auto& stat : stats) {
            if (stat.cpu_percentage > 10.0) {
                LOG_WARNING() << "CPU hotspot detected: " << stat.name
                             << " consuming " << stat.cpu_percentage << "% of CPU time";
            }
        }
    }
    
private:
    FunctionProfile* GetOrCreateProfile(const std::string& function_name) {
        // Try to get existing profile with shared lock first
        {
            std::shared_lock lock(profiles_mutex_);
            auto it = profiles_.find(function_name);
            if (it != profiles_.end()) {
                return it->second.get();
            }
        }
        
        // Create new profile with exclusive lock
        std::unique_lock lock(profiles_mutex_);
        
        // Double-check after acquiring exclusive lock
        auto it = profiles_.find(function_name);
        if (it != profiles_.end()) {
            return it->second.get();
        }
        
        auto profile = std::make_unique<FunctionProfile>();
        profile->function_name = function_name;
        profile->first_call = std::chrono::steady_clock::now();
        
        auto* profile_ptr = profile.get();
        profiles_[function_name] = std::move(profile);
        
        return profile_ptr;
    }
};

// Macro for easy function profiling
#define PROFILE_FUNCTION(detector) \
    CPUHotspotDetector::ScopedProfiler _prof(detector, __FUNCTION__)
```

## I/O Profiling

### Database Query Profiling

**Advanced Database Performance Analysis**
```cpp
// Database query profiler
class DatabaseQueryProfiler {
private:
    struct QueryProfile {
        std::string query_template;
        std::atomic<size_t> execution_count{0};
        std::atomic<std::chrono::nanoseconds::rep> total_time{0};
        std::atomic<std::chrono::nanoseconds::rep> max_time{0};
        std::atomic<size_t> error_count{0};
        std::atomic<size_t> slow_query_count{0};
        std::chrono::steady_clock::time_point first_execution;
        std::chrono::steady_clock::time_point last_execution;
    };
    
    std::unordered_map<std::string, std::unique_ptr<QueryProfile>> query_profiles_;
    std::shared_mutex profiles_mutex_;
    std::chrono::milliseconds slow_query_threshold_{1000}; // 1 second
    
public:
    template<typename QueryFunc>
    auto ProfileQuery(const std::string& query_template, QueryFunc&& func) {
        auto start_time = std::chrono::steady_clock::now();
        
        try {
            auto result = std::forward<QueryFunc>(func)();
            
            auto end_time = std::chrono::steady_clock::now();
            auto duration = end_time - start_time;
            
            RecordQueryExecution(query_template, duration, true);
            
            return result;
        } catch (const std::exception& e) {
            auto end_time = std::chrono::steady_clock::now();
            auto duration = end_time - start_time;
            
            RecordQueryExecution(query_template, duration, false);
            
            LOG_ERROR() << "Database query failed: " << query_template
                       << " error=" << e.what()
                       << " duration=" << duration.count() << "ns";
            
            throw;
        }
    }
    
    void RecordQueryExecution(const std::string& query_template,
                             std::chrono::nanoseconds duration,
                             bool success) {
        QueryProfile* profile = GetOrCreateQueryProfile(query_template);
        
        profile->execution_count.fetch_add(1);
        profile->total_time.fetch_add(duration.count());
        profile->last_execution = std::chrono::steady_clock::now();
        
        if (!success) {
            profile->error_count.fetch_add(1);
        }
        
        if (duration > slow_query_threshold_) {
            profile->slow_query_count.fetch_add(1);
            
            LOG_WARNING() << "Slow query detected: " << query_template
                         << " duration=" << duration.count() << "ns";
        }
        
        // Update max time atomically
        auto current_max = profile->max_time.load();
        while (duration.count() > current_max && 
               !profile->max_time.compare_exchange_weak(current_max, duration.count())) {
            // Retry if another thread updated max_time
        }
    }
    
    void GenerateQueryPerformanceReport() {
        std::shared_lock lock(profiles_mutex_);
        
        struct QueryStats {
            std::string template_name;
            size_t execution_count;
            std::chrono::nanoseconds total_time;
            std::chrono::nanoseconds avg_time;
            std::chrono::nanoseconds max_time;
            size_t error_count;
            size_t slow_query_count;
            double error_rate;
            double slow_query_rate;
        };
        
        std::vector<QueryStats> stats;
        
        for (const auto& [template_name, profile] : query_profiles_) {
            auto execution_count = profile->execution_count.load();
            if (execution_count == 0) continue;
            
            auto total_time = std::chrono::nanoseconds{profile->total_time.load()};
            auto max_time = std::chrono::nanoseconds{profile->max_time.load()};
            auto error_count = profile->error_count.load();
            auto slow_query_count = profile->slow_query_count.load();
            
            auto avg_time = total_time / execution_count;
            auto error_rate = static_cast<double>(error_count) / execution_count * 100.0;
            auto slow_query_rate = static_cast<double>(slow_query_count) / execution_count * 100.0;
            
            stats.push_back(QueryStats{
                .template_name = template_name,
                .execution_count = execution_count,
                .total_time = total_time,
                .avg_time = avg_time,
                .max_time = max_time,
                .error_count = error_count,
                .slow_query_count = slow_query_count,
                .error_rate = error_rate,
                .slow_query_rate = slow_query_rate
            });
        }
        
        // Sort by total execution time
        std::sort(stats.begin(), stats.end(),
                 [](const auto& a, const auto& b) {
                     return a.total_time > b.total_time;
                 });
        
        LOG_INFO() << "Database Query Performance Report:";
        for (const auto& stat : stats) {
            LOG_INFO() << "Query: " << stat.template_name
                      << " executions=" << stat.execution_count
                      << " total_time=" << stat.total_time.count() << "ns"
                      << " avg_time=" << stat.avg_time.count() << "ns"
                      << " max_time=" << stat.max_time.count() << "ns"
                      << " error_rate=" << std::fixed << std::setprecision(2) << stat.error_rate << "%"
                      << " slow_query_rate=" << stat.slow_query_rate << "%";
            
            // Highlight problematic queries
            if (stat.error_rate > 5.0) {
                LOG_WARNING() << "High error rate query: " << stat.template_name;
            }
            
            if (stat.slow_query_rate > 10.0) {
                LOG_WARNING() << "Frequently slow query: " << stat.template_name;
            }
        }
    }
    
private:
    QueryProfile* GetOrCreateQueryProfile(const std::string& query_template) {
        // Try to get existing profile with shared lock first
        {
            std::shared_lock lock(profiles_mutex_);
            auto it = query_profiles_.find(query_template);
            if (it != query_profiles_.end()) {
                return it->second.get();
            }
        }
        
        // Create new profile with exclusive lock
        std::unique_lock lock(profiles_mutex_);
        
        // Double-check after acquiring exclusive lock
        auto it = query_profiles_.find(query_template);
        if (it != query_profiles_.end()) {
            return it->second.get();
        }
        
        auto profile = std::make_unique<QueryProfile>();
        profile->query_template = query_template;
        profile->first_execution = std::chrono::steady_clock::now();
        
        auto* profile_ptr = profile.get();
        query_profiles_[query_template] = std::move(profile);
        
        return profile_ptr;
    }
};
```

## GDB Integration for Advanced Debugging

### Custom GDB Commands

**userver-specific GDB Integration**
```cpp
// GDB debugging utilities
class GDBIntegration {
public:
    // Enable GDB auto-loading for userver scripts
    static void SetupGDBEnvironment() {
        // Instructions for ~/.gdbinit setup
        LOG_INFO() << "GDB Setup Instructions:";
        LOG_INFO() << "Add to ~/.gdbinit: add-auto-load-safe-path /path/to/binary";
        LOG_INFO() << "Or for all files: add-auto-load-safe-path /";
    }
    
    // Coroutine debugging helpers
    static void DumpCoroutineState() {
        // This would be called from GDB context
        LOG_INFO() << "Use GDB commands:";
        LOG_INFO() << "  utask list                    - List all tasks";
        LOG_INFO() << "  utask apply <task> <cmd>      - Execute command on task";
        LOG_INFO() << "  utask apply all backtrace     - Get all task backtraces";
        LOG_INFO() << "  utask apply task_1 backtrace  - Get specific task backtrace";
    }
    
    // Pretty printer information
    static void EnablePrettyPrinters() {
        LOG_INFO() << "userver provides pretty printers for:";
        LOG_INFO() << "  - formats::json::Value";
        LOG_INFO() << "  - formats::yaml::Value";
        LOG_INFO() << "  - formats::bson::Value";
        LOG_INFO() << "  - engine::Task";
        LOG_INFO() << "  - tracing::Span";
    }
    
    // Stack usage monitoring
    static void ConfigureStackMonitoring() {
        LOG_INFO() << "Stack usage monitoring configuration:";
        LOG_INFO() << "Environment: USERVER_GTEST_ENABLE_STACK_USAGE_MONITOR=0";
        LOG_INFO() << "Static config: coro_pool.stack_usage_monitor_enabled: false";
        LOG_INFO() << "Build option: USERVER_FEATURE_STACK_USAGE_MONITOR=OFF";
    }
};
```

## Integration Points

**Cross-References**
- [`troubleshooting-workflows.md`](./troubleshooting-workflows.md) - General debugging workflows
- [`performance-analysis.md`](./performance-analysis.md) - Performance debugging integration
- [`error-investigation.md`](./error-investigation.md) - Error profiling techniques

**Memory Bank References**
- [`performance-research.md`](../../memory-bank/research/performance-research.md) - Performance research
- [`advanced-monitoring`](../../memory-bank/specialized/advanced-monitoring/) - Monitoring integration
- [`framework-core.md`](../../memory-bank/main/framework-core.md) - Core framework profiling

## Best Practices

### Profiling Strategy
- Profile in production-like environments
- Use sampling to reduce overhead
- Focus on critical code paths
- Regular profiling sessions

### Tool Integration
- Combine multiple profiling techniques
- Automate profiling data collection
- Integrate with monitoring systems
- Document profiling procedures

### Performance Optimization
- Profile before optimizing
- Measure optimization impact
- Maintain performance baselines
- Regular performance reviews