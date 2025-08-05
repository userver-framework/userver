# Troubleshooting Help and Problem Resolution

## Overview

Comprehensive troubleshooting guidance for common userver issues, systematic debugging approaches, and problem resolution strategies based on official FAQ and community knowledge.

## Common Issue Categories and Solutions

### Service Startup and Build Issues

**Framework Build Problems**
```yaml
issue: "The framework does not build"
common_causes:
  - "Windows OS (not supported - use WSL)"
  - "Missing dependencies"
  - "Incompatible PostgreSQL version"
  - "Disabled required modules"

solutions:
  - "Use WSL on Windows systems"
  - "Check build dependencies documentation"
  - "Verify PostgreSQL version compatibility"
  - "Review CMake build options and enable required modules"
  
diagnostic_steps:
  - "Check build logs for specific error messages"
  - "Verify system requirements and dependencies"
  - "Try disabling unused modules with CMake options"
  - "Consult PostgreSQL versions documentation"
```

**Service Won't Start**
```cpp
// Common startup issues and diagnostics
void DiagnoseStartupIssues() {
    // Check configuration file syntax
    // Verify component dependencies
    // Ensure required ports are available
    // Check file permissions and paths
}
```

### Runtime Crashes and Errors

**Service Terminated/Aborted/SIGSEGV**
```yaml
diagnostic_approach:
  step_1: "Check service logs for error messages and hints"
  step_2: "Analyze core dump or stacktrace"
  step_3: "Look for common patterns"
  
common_patterns:
  std_terminate:
    cause: "Exception thrown from noexcept function"
    solution: "Add try-catch blocks in noexcept functions"
    
  async_lifetime_issues:
    cause: "Captured references outlive async operations"
    solution: "Capture by value or ensure proper lifetime management"
    
  component_lifecycle:
    cause: "Component cleanup order issues"
    solution: "Review component dependencies and initialization order"
```

**Exception Handling Patterns**
```cpp
// ✅ Correct: Proper exception handling in noexcept functions
void NoexceptFunction() noexcept {
    try {
        RiskyOperation();
    } catch (const std::exception& e) {
        LOG_ERROR() << "Error in noexcept function: " << e.what();
        // Handle gracefully without throwing
    }
}

// ✅ Correct: Safe async operation lifetime management
engine::TaskWithResult<void> SafeAsyncOperation() {
    std::string data = "important data";
    
    // Capture by value to ensure lifetime
    auto task = utils::Async("process", [data]() {
        return ProcessData(data);  // Safe: data is owned by lambda
    });
    
    co_await task;  // data lifetime is guaranteed
}
```

**Core Dump Analysis**
```bash
# Analyzing core dumps with GDB
gdb ./service core.dump

# Get stack trace for all threads
(gdb) thread apply all bt

# Look for specific patterns
(gdb) info threads
(gdb) thread 1
(gdb) bt full

# Check for std::terminate calls
(gdb) info registers
(gdb) disassemble
```

### Performance and Hanging Issues

**Service Waiting/Hanging**
```yaml
diagnostic_strategy:
  logs_analysis:
    - "Check for blocking operations in main task processor"
    - "Look for deadlock patterns in mutex usage"
    - "Identify resource exhaustion messages"
    
  cpu_analysis:
    command: "top -b -H -n 3"
    purpose: "Identify high CPU threads"
    interpretation: "userver threads have descriptive names"
    
  stacktrace_analysis:
    command: "gdb -batch -ex 'thread apply all bt full' -p PID"
    purpose: "Get detailed thread information"
    look_for: "futex_wait patterns indicating blocking"
    
  metrics_analysis:
    key_metrics:
      - "engine.coro-pool.coroutines.active (task processor load)"
      - "major_pagefaults (blocking filesystem operations)"
      - "Connection pool metrics (database bottlenecks)"
```

**Performance Bottleneck Identification**
```cpp
// Common performance issues and solutions

// ❌ Problem: Blocking operation in main task processor
void SlowHandler() {
    std::ifstream file("data.txt");  // Blocks entire task processor!
    // Process file...
}

// ✅ Solution: Move to fs-task-processor
engine::TaskWithResult<void> FastHandler() {
    auto data = co_await utils::Async("read_file",
                                     utils::TaskProcessor::Get("fs-task-processor"),
                                     []() {
        std::ifstream file("data.txt");
        // Read file content
        return content;
    });
    // Process data...
}

// ❌ Problem: Sequential async operations
engine::TaskWithResult<CombinedResult> SlowCombination() {
    auto result1 = co_await FetchData1();  // Wait unnecessarily
    auto result2 = co_await FetchData2();  // Could be concurrent
    return Combine(result1, result2);
}

// ✅ Solution: Concurrent async operations
engine::TaskWithResult<CombinedResult> FastCombination() {
    auto task1 = utils::Async("fetch1", []() { return FetchData1(); });
    auto task2 = utils::Async("fetch2", []() { return FetchData2(); });
    
    auto result1 = co_await task1;
    auto result2 = co_await task2;
    return Combine(result1, result2);
}
```

### Database-Related Issues

**PostgreSQL Connection Problems**
```yaml
network_timeout_error:
  symptom: "Statement XXXX network timeout error"
  explanation: "Database server failed to answer in time"
  formula: "acquire connection + execute query <= network timeout"
  solutions:
    - "Increase network timeout in dynamic config"
    - "Optimize query performance"
    - "Check connection pool size"
    - "Monitor connection acquisition time"

statement_canceled_error:
  symptom: "Statement XXXX was canceled"
  cause: "Statement timeout exceeded"
  solutions:
    - "Increase statement timeout in dynamic config"
    - "Optimize query performance"
    - "Review query complexity"
    - "Check for long-running transactions"

connection_pool_exhaustion:
  symptoms:
    - "High values in *.acquire-connection metrics"
    - "Timeouts close to network timeout"
    - "Pool-related error messages"
  solutions:
    - "Increase connection pool size"
    - "Optimize query performance"
    - "Review connection usage patterns"
    - "Monitor connection lifecycle metrics"
```

**Database Performance Issues**
```cpp
// Diagnostic queries and monitoring
class DatabaseDiagnostics {
public:
    void AnalyzePerformanceMetrics() {
        // Check key PostgreSQL metrics:
        // - *.pool.* (connection pool health)
        // - *.acquire-connection (connection acquisition time)
        // - *.return-to-pool (connection cleanup time)
        // - *.busy (query execution time)
        
        LOG_INFO() << "Database performance analysis:"
                   << logging::LogExtra::Key("pool_size", GetPoolSize())
                   << logging::LogExtra::Key("active_connections", GetActiveConnections())
                   << logging::LogExtra::Key("avg_query_time", GetAverageQueryTime());
    }
    
    engine::TaskWithResult<void> OptimizeSlowQuery() {
        // Use EXPLAIN ANALYZE for query optimization
        auto result = co_await pg_cluster_->Execute(
            storages::postgres::ClusterHostType::kSlave,
            "EXPLAIN ANALYZE SELECT * FROM users WHERE complex_condition = $1",
            condition_value
        );
        
        // Analyze execution plan and optimize accordingly
        AnalyzeExecutionPlan(result);
    }
};
```

**Timestamp and Timezone Issues**
```sql
-- Common PostgreSQL timestamp issues
-- ❌ Problem: timestamp WITHOUT time zone inconsistencies
CREATE TABLE events (
    id SERIAL PRIMARY KEY,
    created_at TIMESTAMP  -- Problematic: no timezone info
);

-- ✅ Solution: Always use timestamp WITH time zone
CREATE TABLE events (
    id SERIAL PRIMARY KEY,
    created_at TIMESTAMPTZ  -- Correct: includes timezone
);

-- For existing timestamp columns, convert properly:
SELECT created_at AT TIME ZONE 'UTC' FROM events;
```

### Memory and Resource Issues

**Memory Leaks and High Memory Usage**
```cpp
// Memory leak detection and prevention
class MemoryDiagnostics {
public:
    void EnableMemoryProfiling() {
        // Enable stacktrace for memory allocations
        // Use USERVER_FEATURE_STACKTRACE=ON
        
        LOG_ERROR() << "Memory allocation issue detected"
                   << logging::LogExtra::Stacktrace();
    }
    
    void AnalyzeMemoryPatterns() {
        // Common memory leak patterns:
        // 1. Circular references in shared_ptr
        // 2. Uncanceled async operations
        // 3. Large object accumulation in caches
        // 4. Resource leaks in exception paths
    }
};

// ✅ Correct: RAII resource management
class ResourceManager {
    std::unique_ptr<Resource> resource_;
    
public:
    ResourceManager() : resource_(std::make_unique<Resource>()) {
        // Resource acquired
    }
    
    ~ResourceManager() {
        // Automatic cleanup - no manual resource management needed
    }
};

// ❌ Avoid: Manual resource management
class BadResourceManager {
    Resource* resource_;
    
public:
    BadResourceManager() : resource_(new Resource()) {}
    
    // Missing destructor - memory leak!
    // Manual cleanup required - error-prone
};
```

**Deadlock Detection and Resolution**
```cpp
// Deadlock prevention strategies
class DeadlockPrevention {
public:
    void AnalyzeLockOrdering() {
        // Document and enforce consistent lock ordering
        // Thread 1: Lock A -> Lock B
        // Thread 2: Lock A -> Lock B (same order - no deadlock)
        
        // ❌ Dangerous: Inconsistent lock ordering
        // Thread 1: Lock A -> Lock B
        // Thread 2: Lock B -> Lock A (potential deadlock!)
    }
    
    void EnableMutexDebugging() {
        // Use debug builds with detailed mutex logging
        LOG_DEBUG() << "Acquiring mutex: " << mutex_name_;
        std::lock_guard lock(mutex_);
        LOG_DEBUG() << "Mutex acquired: " << mutex_name_;
        
        // Mutex automatically released when lock goes out of scope
    }
    
    void UseTimeoutLocks() {
        // Use timed locks to detect potential deadlocks
        if (mutex_.try_lock_for(std::chrono::seconds(5))) {
            // Process with lock held
            mutex_.unlock();
        } else {
            LOG_WARNING() << "Failed to acquire lock - potential deadlock";
        }
    }
};
```

## Systematic Debugging Approach

### Issue Classification Framework

**Step 1: Categorize the Issue**
```yaml
issue_types:
  startup_issues:
    - "Build failures"
    - "Configuration errors"
    - "Dependency problems"
    - "Port conflicts"
    
  runtime_crashes:
    - "Segmentation faults"
    - "Assertion failures"
    - "Exception propagation"
    - "Resource exhaustion"
    
  performance_issues:
    - "High latency"
    - "Low throughput"
    - "Memory leaks"
    - "CPU spikes"
    
  functional_issues:
    - "Incorrect behavior"
    - "Data corruption"
    - "Logic errors"
    - "Integration failures"
```

**Step 2: Gather Diagnostic Information**
```bash
# Essential diagnostic data collection
# 1. Service logs
tail -f /var/log/service/app.log

# 2. System metrics
top -b -H -n 3
iostat -x 1 5
free -h

# 3. Process information
ps aux | grep service_name
lsof -p PID

# 4. Network status
netstat -tlnp | grep :8080
ss -tlnp | grep :8080

# 5. Core dump analysis (if available)
gdb ./service core.dump
```

**Step 3: Apply Systematic Investigation**
```cpp
// Debugging workflow implementation
class SystematicDebugging {
public:
    void InvestigateIssue() {
        // 1. Reproduce the issue consistently
        auto reproduction_case = CreateMinimalReproduction();
        
        // 2. Enable detailed logging
        EnableDebugLogging();
        
        // 3. Add tracing spans to suspect code
        AddTracingToSuspectCode();
        
        // 4. Monitor key metrics
        SetupMonitoringDashboard();
        
        // 5. Analyze patterns and correlations
        AnalyzePatterns();
    }
    
private:
    void EnableDebugLogging() {
        // Enable debug level logging for specific components
        // Use USERVER_LOG_DYNAMIC_DEBUG for runtime control
    }
    
    void AddTracingToSuspectCode() {
        // Add tracing spans to identify bottlenecks
        tracing::Span span("suspect_operation");
        span.AddTag("input_size", input.size());
        // ... operation code ...
        span.AddTag("result_count", results.size());
    }
};
```

### Debugging Tools and Techniques

**GDB Integration for userver**
```bash
# Start debugging session
gdb ./service
(gdb) run --config=debug.yaml

# userver-specific GDB commands
(gdb) utask list                    # List all tasks
(gdb) utask apply task_1 backtrace  # Get task backtrace
(gdb) utask apply all print "Debug info"  # Execute on all tasks

# Analyze coroutine state
(gdb) info threads
(gdb) thread apply all bt full
```

**Dynamic Configuration for Debugging**
```json
{
  "USERVER_LOG_DYNAMIC_DEBUG": [
    {
      "location": "src/handlers/problematic_handler.cpp:42",
      "level": "debug"
    }
  ],
  "USERVER_TASK_PROCESSOR_PROFILER_ENABLED": true,
  "USERVER_CANCEL_BEHAVIOUR": "kCancel"
}
```

**Performance Profiling**
```cpp
// Enable performance profiling in static config
/*
task_processors:
  main-task-processor:
    task-trace:
      every: 1
      max-context-switch-count: 1000
      logger: tracer
*/

class PerformanceProfiler {
public:
    void AnalyzeContextSwitches() {
        // Look for patterns in trace logs:
        // - Tasks switching too frequently
        // - Long delays between state changes
        // - Blocking operations in hot paths
    }
    
    void ProfileMemoryUsage() {
        // Use built-in memory profiling
        // Monitor heap growth patterns
        // Identify memory hotspots
    }
};
```

## Problem Resolution Strategies

### Quick Fixes for Common Issues

**Service Won't Start**
```yaml
checklist:
  - "Verify configuration file syntax (YAML validation)"
  - "Check port availability (lsof -i :8080)"
  - "Ensure file permissions (config files, log directories)"
  - "Validate component dependencies"
  - "Check disk space and memory availability"
```

**High CPU Usage**
```yaml
investigation_steps:
  - "Identify high CPU threads with top -H"
  - "Check for infinite loops in business logic"
  - "Look for blocking operations in main task processor"
  - "Analyze task processor metrics"
  - "Review recent code changes"
```

**Database Connection Issues**
```yaml
resolution_steps:
  - "Check database server availability"
  - "Verify connection string and credentials"
  - "Review connection pool configuration"
  - "Monitor connection pool metrics"
  - "Check network connectivity and firewall rules"
```

### Advanced Troubleshooting Techniques

**Chaos Testing Integration**
```cpp
// Use chaos testing to identify resilience issues
class ChaosTestingDiagnostics {
public:
    void SimulateNetworkIssues() {
        // Introduce network delays and failures
        // Observe service behavior under stress
    }
    
    void SimulateDatabaseFailures() {
        // Test database connection handling
        // Verify graceful degradation
    }
    
    void SimulateResourceExhaustion() {
        // Test memory and CPU limits
        // Verify resource cleanup
    }
};
```

**Custom Metrics for Debugging**
```cpp
class DebuggingMetrics {
    utils::statistics::Counter debug_events_;
    utils::statistics::Histogram operation_timing_;
    utils::statistics::Gauge resource_usage_;
    
public:
    void RecordDebugEvent(const std::string& event_type) {
        debug_events_.Increment();
        LOG_DEBUG() << "Debug event recorded: " << event_type;
    }
    
    void MeasureOperationTiming(std::chrono::milliseconds duration) {
        operation_timing_.Account(duration.count());
    }
};
```

## Community and Support Resources

### When to Seek Help

**Internal Resolution First**
```yaml
try_first:
  - "Check official FAQ documentation"
  - "Search existing GitHub issues"
  - "Review similar problems in documentation"
  - "Consult team knowledge base"
  - "Apply systematic debugging approach"
```

**Community Support Channels**
```yaml
support_channels:
  telegram_english: "https://t.me/userver_en"
  telegram_russian: "https://t.me/userver_ru"
  github_issues: "https://github.com/userver-framework/userver/issues"
  
information_to_provide:
  - "userver version and build configuration"
  - "Operating system and environment details"
  - "Minimal reproduction case"
  - "Relevant log excerpts"
  - "Configuration files (sanitized)"
  - "Steps already attempted"
```

### Creating Effective Bug Reports

**Bug Report Template**
```markdown
## Environment
- userver version: 
- OS: 
- Compiler: 
- Build configuration: 

## Problem Description
Brief description of the issue

## Steps to Reproduce
1. Step one
2. Step two
3. Step three

## Expected Behavior
What should happen

## Actual Behavior
What actually happens

## Logs and Stack Traces
```
[relevant log excerpts]
```

## Additional Context
Any other relevant information
```

## Prevention Strategies

### Proactive Monitoring
```cpp
// Implement comprehensive health checks
class ProactiveMonitoring {
public:
    void SetupHealthChecks() {
        // Database connectivity
        // External service availability
        // Resource usage thresholds
        // Performance baselines
    }
    
    void ConfigureAlerting() {
        // Critical error alerts
        // Performance degradation warnings
        // Resource exhaustion notifications
        // Dependency failure alerts
    }
};
```

### Code Quality Practices
```yaml
prevention_practices:
  code_review:
    - "Review async operation patterns"
    - "Check resource management (RAII)"
    - "Verify error handling completeness"
    - "Validate configuration usage"
    
  testing:
    - "Unit tests for critical paths"
    - "Integration tests for external dependencies"
    - "Load testing for performance validation"
    - "Chaos testing for resilience verification"
    
  monitoring:
    - "Comprehensive metrics collection"
    - "Structured logging with context"
    - "Distributed tracing for request flows"
    - "Regular performance baseline updates"
```

### Documentation and Knowledge Sharing
```yaml
knowledge_management:
  runbooks:
    - "Common issue resolution procedures"
    - "Emergency response protocols"
    - "Performance tuning guidelines"
    - "Deployment troubleshooting steps"
    
  team_practices:
    - "Regular post-mortem reviews"
    - "Knowledge sharing sessions"
    - "Documentation updates after incidents"
    - "Best practices evolution"
```

This troubleshooting guide provides systematic approaches to identifying, diagnosing, and resolving common userver issues, with emphasis on proactive monitoring and prevention strategies.