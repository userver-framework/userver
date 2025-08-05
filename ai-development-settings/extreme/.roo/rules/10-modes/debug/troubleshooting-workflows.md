# Troubleshooting Workflows

## Overview

Systematic debugging approaches for userver services, providing step-by-step procedures for identifying and resolving issues in production and development environments.

## Core Debugging Methodology

### 1. Issue Classification

**Immediate Assessment**
```cpp
// Step 1: Classify the issue type
enum class IssueType {
    kPerformance,     // Slow responses, high latency
    kCrash,          // Service crashes, core dumps
    kLogic,          // Incorrect behavior, wrong results
    kResource,       // Memory leaks, high CPU usage
    kNetwork,        // Connection issues, timeouts
    kDatabase        // Query failures, connection problems
};
```

**Severity Levels**
- **Critical**: Service down, data corruption
- **High**: Performance degradation, partial functionality loss
- **Medium**: Minor issues, workarounds available
- **Low**: Cosmetic issues, future improvements

### 2. Information Gathering

**Essential Data Collection**
```yaml
# Debug information checklist
debug_info:
  logs:
    - application_logs: /var/log/service/app.log
    - error_logs: /var/log/service/error.log
    - trace_logs: /var/log/service/trace.log
  metrics:
    - cpu_usage: "Check system metrics"
    - memory_usage: "Monitor heap and stack"
    - request_latency: "P50, P95, P99 percentiles"
  environment:
    - service_version: "Git commit hash"
    - configuration: "Static and dynamic configs"
    - dependencies: "Database, external services"
```

### 3. Systematic Investigation

**Step-by-Step Process**

1. **Reproduce the Issue**
   ```cpp
   // Create minimal reproduction case
   TEST(DebugTest, ReproduceIssue) {
       // Set up minimal environment
       auto service = CreateTestService();
       
       // Execute problematic scenario
       auto result = service.ProcessRequest(problematic_input);
       
       // Verify issue occurs
       EXPECT_THAT(result, HasIssue());
   }
   ```

2. **Enable Debug Logging**
   ```yaml
   # Static config for debug mode
   components_manager:
     components:
       logging:
         loggers:
           default:
             level: debug  # Enable debug logs
             file_path: /tmp/debug.log
   ```

3. **Add Tracing Spans**
   ```cpp
   void ProblematicFunction() {
       tracing::Span span("problematic_function");
       span.AddTag("input_size", input.size());
       
       // Add detailed tracing
       {
           tracing::Span sub_span("data_processing");
           ProcessData();
       }
       
       span.AddTag("result_count", results.size());
   }
   ```

## Common Issue Patterns

### Performance Issues

**Symptoms**
- High response times
- CPU spikes
- Memory growth
- Context switch storms

**Investigation Steps**
```cpp
// 1. Enable performance profiling
void EnablePerformanceProfiling() {
    // Static config modification
    /*
    task_processors:
      main-task-processor:
        task-trace:
          every: 1
          max-context-switch-count: 1000
          logger: tracer
    */
}

// 2. Analyze context switches
void AnalyzeContextSwitches() {
    // Look for patterns in trace logs:
    // - Tasks switching too frequently
    // - Long delays between state changes
    // - Blocking operations in hot paths
}
```

**Resolution Patterns**
```cpp
// Optimize blocking operations
class OptimizedService {
public:
    // Before: Blocking operation
    void SlowMethod() {
        auto result = database_.ExecuteBlocking(query);
        ProcessResult(result);
    }
    
    // After: Async operation
    engine::TaskWithResult<void> FastMethod() {
        auto result = co_await database_.ExecuteAsync(query);
        ProcessResult(result);
    }
};
```

### Memory Issues

**Detection**
```cpp
// Enable memory profiling
void EnableMemoryProfiling() {
    // Use USERVER_FEATURE_STACKTRACE=ON
    // Monitor with logging::LogExtra::Stacktrace()
    
    LOG_ERROR() << "Memory allocation issue detected"
                << logging::LogExtra::Stacktrace();
}
```

**Common Patterns**
- Memory leaks in async operations
- Circular references in shared_ptr
- Large object accumulation

### Deadlock Detection

**Systematic Approach**
```cpp
// 1. Enable mutex debugging
void EnableMutexDebugging() {
    // Use debug build with detailed logging
    LOG_DEBUG() << "Acquiring mutex: " << mutex_name;
    std::lock_guard lock(mutex_);
    LOG_DEBUG() << "Mutex acquired: " << mutex_name;
}

// 2. Analyze lock ordering
void AnalyzeLockOrdering() {
    // Document lock hierarchy
    // Thread 1: Lock A -> Lock B
    // Thread 2: Lock B -> Lock A (DEADLOCK!)
}
```

## Advanced Debugging Techniques

### GDB Integration

**Setup**
```bash
# Enable GDB auto-loading
echo "add-auto-load-safe-path /path/to/binary" >> ~/.gdbinit

# Start debugging session
gdb ./service
(gdb) run --config=debug.yaml
```

**Coroutine Debugging**
```gdb
# List all tasks
(gdb) utask list

# Get backtrace of specific task
(gdb) utask apply task_1 backtrace

# Execute command on all tasks
(gdb) utask apply all print "Debug info"
```

### Dynamic Configuration

**Runtime Debugging**
```cpp
// Enable debug logging at runtime
void EnableRuntimeDebugging() {
    // Use USERVER_LOG_DYNAMIC_DEBUG dynamic config
    /*
    {
      "USERVER_LOG_DYNAMIC_DEBUG": [
        {
          "location": "src/handlers/problematic_handler.cpp:42",
          "level": "debug"
        }
      ]
    }
    */
}
```

## Troubleshooting Checklists

### Pre-Investigation Checklist
- [ ] Collect service version and configuration
- [ ] Gather recent logs (last 1 hour)
- [ ] Check system resources (CPU, memory, disk)
- [ ] Verify external dependencies status
- [ ] Document reproduction steps

### Investigation Checklist
- [ ] Enable appropriate logging level
- [ ] Add tracing spans to suspect code
- [ ] Set up monitoring dashboards
- [ ] Create minimal reproduction case
- [ ] Document findings and hypotheses

### Resolution Checklist
- [ ] Implement fix with proper testing
- [ ] Verify fix resolves original issue
- [ ] Check for performance regressions
- [ ] Update documentation and runbooks
- [ ] Plan monitoring improvements

## Integration Points

**Cross-References**
- [`performance-analysis.md`](./performance-analysis.md) - Performance debugging techniques
- [`error-investigation.md`](./error-investigation.md) - Error analysis patterns
- [`profiling-techniques.md`](./profiling-techniques.md) - Advanced profiling methods
- [`monitoring-debug.md`](./monitoring-debug.md) - Debug monitoring setup

**Memory Bank References**
- [`troubleshooting-guide.md`](../../memory-bank/main/troubleshooting-guide.md) - General troubleshooting patterns
- [`async-programming.md`](../../memory-bank/main/async-programming.md) - Async debugging considerations
- [`advanced-monitoring`](../../memory-bank/specialized/advanced-monitoring/) - Monitoring integration

## Best Practices

### Documentation
- Always document reproduction steps
- Include environment details
- Record investigation timeline
- Share findings with team

### Prevention
- Implement comprehensive logging
- Add health checks and metrics
- Use structured error handling
- Regular performance testing

### Collaboration
- Use shared debugging sessions
- Maintain debugging runbooks
- Regular post-mortem reviews
- Knowledge sharing sessions