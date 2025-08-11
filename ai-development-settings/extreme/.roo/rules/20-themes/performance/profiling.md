# Performance Profiling Guide

## Overview
Comprehensive guide for profiling userver applications to identify performance bottlenecks, context switches, and optimization opportunities.

## Context Switch Profiling

### Configuration Setup
To enable context switch profiling, configure the `task-trace` options in your static configuration file:

```yaml
task_processors:
  main-task-processor:
    worker_threads: 4
    task-trace:
      every: 1
      max-context-switch-count: 1000
      logger: tracer

components:
  logging:
    loggers:
      tracer:
        file_path: $tracer_log_path
        file_path#fallback: '@null'
        level: $tracer_level  # set to debug to get stacktraces
        level#fallback: info
```

### Build Requirements
- Ensure service is built with debug information
- Enable cmake option `USERVER_FEATURE_STACKTRACE` to `ON`
- Use modern `libbacktrace` library for proper stacktrace demangling

### Analyzing Profiling Output

#### Basic Profiling Logs
Example profiling log output:
```
INFO Task 7F3081833C00 changed state to kSuspended, delay = 73us
INFO Task 7F3081833600 changed state to kRunning, delay = 273us
INFO Task 7F3081833C00 changed state to kQueued, delay = 2736194us
```

Suspicious patterns to look for:
1. Long delays when changing state from `kSuspended` to `kQueued` - indicates waiting for I/O or synchronization
2. Frequent state changes - suggests excessive context switching

#### Stacktrace Analysis
Enable debug level logging to get stacktraces:
```
INFO Task 7F3081833C00 changed state to kQueued, delay = 101us span_id=e806a3e2857714b3 stacktrace=
 0# userver::engine::impl::TaskContext::TraceStateTransition() at task_context.cpp:729
 1# userver::engine::impl::TaskContext::Schedule() at task_context.cpp:672
 2# userver::engine::impl::TaskContext::Wakeup() at task_context.cpp:502
 3# userver::engine::Mutex::unlock() at mutex.cpp:81
```

Stacktraces help identify:
- Specific functions causing delays
- Synchronization primitive contention
- I/O wait locations

## Performance Monitoring

### Service Metrics Endpoint
Access service metrics via the monitoring endpoint:
```bash
curl 'http://localhost:8086/service/monitor?format=prometheus'
```

Available metrics include:
- Engine performance metrics
- HTTP handler latencies
- Database operation timings
- Cache hit/miss ratios
- Task processor utilization

### Key Metrics to Monitor
- **Context switch frequency** - High frequency may indicate inefficient task scheduling
- **Task queue depths** - Deep queues suggest resource contention
- **I/O operation latencies** - Long latencies indicate bottlenecks
- **Memory allocation patterns** - Frequent allocations may need optimization

## Profiling Tools Integration

### CPU Profiling
Use `perf` for CPU profiling:
```bash
perf record -g ./your-service
perf report
```

### Memory Profiling
Use Valgrind or heaptrack for memory profiling:
```bash
valgrind --tool=massif ./your-service
heaptrack ./your-service
```

### Flame Graphs
Generate flame graphs for visual performance analysis:
```bash
perf record -g ./your-service
perf script | stackcollapse-perf.pl | flamegraph.pl > perf.svg
```

## Profiling Best Practices

### Production Profiling
- Use sampling to minimize overhead
- Profile during peak load periods
- Monitor impact of profiling on service performance
- Use appropriate log levels to balance detail with performance

### Development Profiling
- Profile regularly during development
- Compare performance before and after changes
- Use unit tests with profiling enabled
- Document performance characteristics

### Common Profiling Scenarios

#### High CPU Usage
- Check for busy loops or inefficient algorithms
- Look for excessive context switching
- Analyze stacktraces for hot paths
- Monitor task processor utilization

#### High Memory Usage
- Use memory profiling tools
- Check for memory leaks
- Analyze allocation patterns
- Monitor cache memory usage

#### Slow Response Times
- Profile individual request handling
- Check database query performance
- Analyze network I/O patterns
- Monitor external service calls

## Troubleshooting

### Common Issues

#### Unreadable Traces
**Problem**: Traces lack function or file names
**Solution**: 
- Verify `USERVER_FEATURE_STACKTRACE` is enabled
- Ensure debug information is not stripped
- Check `libbacktrace` library version

#### Performance Impact
**Problem**: Profiling affects service performance
**Solution**:
- Use sampling (`every: N` configuration)
- Reduce log level
- Profile during off-peak hours
- Use lightweight profiling options

### Profiling Checklist
- [ ] Service built with debug information
- [ ] Stacktrace feature enabled
- [ ] Appropriate task-trace configuration
- [ ] Separate logger for profiling output
- [ ] Debug symbols available
- [ ] Monitoring endpoint accessible
- [ ] Baseline performance metrics collected