# Debug Mode: Monitoring and Metrics Analysis

## Overview

This document provides comprehensive guidelines for using userver's monitoring and metrics system during debugging sessions. The framework provides extensive built-in metrics and allows custom metrics for detailed system observability.

## Core Monitoring Components

### ServerMonitor Component

The [`server::handlers::ServerMonitor`](https://userver.tech/d9/dac/md_en_2userver_2service__monitor.html) component exposes a REST API for metrics at a dedicated monitoring address.

```yaml
# Configuration example
components_manager:
  components:
    server:
      listener-monitor:
        port: 8086
        task_processor: monitor-task-processor
```

### Metrics Access Patterns

```bash
# Fetch all metrics in Prometheus format
curl 'http://localhost:8086/service/monitor?format=prometheus'

# Fetch metrics in Graphite format
curl 'http://localhost:8086/service/monitor?format=graphite'

# Fetch human-readable format (default)
curl 'http://localhost:8086/service/monitor'
```

## Built-in Metrics Categories

### Engine Metrics

Monitor core framework performance:

```
# Task processor metrics
engine.task-processors.tasks.queued: task_processor=main-task-processor GAUGE 0
engine.task-processors.tasks.running: task_processor=main-task-processor GAUGE 2
engine.task-processors.tasks.cancelled: task_processor=main-task-processor RATE 0
engine.task-processors.tasks.created: task_processor=main-task-processor RATE 15.2

# Thread pool metrics
engine.task-processors.threads.active: task_processor=main-task-processor GAUGE 4
engine.task-processors.threads.total: task_processor=main-task-processor GAUGE 8
```

### HTTP Server Metrics

Track HTTP handler performance:

```
# Request metrics
http.handler.requests: handler=hello, http_code=200 RATE 12.5
http.handler.request-time: handler=hello HIST [0.001, 0.005, 0.010, ...]

# Connection metrics
http.connections.active: GAUGE 5
http.connections.opened: RATE 2.1
http.connections.closed: RATE 2.0
```

### HTTP Client Metrics

Monitor outbound HTTP requests:

```
# Client request metrics
http.client.requests: url=https://api.example.com, http_code=200 RATE 8.3
http.client.request-time: url=https://api.example.com HIST [0.050, 0.100, ...]
http.client.connection-errors: url=https://api.example.com RATE 0.1
```

### Database Metrics

#### PostgreSQL Metrics

```
# Connection pool metrics
postgresql.connections.active: database=main_db GAUGE 3
postgresql.connections.used: database=main_db GAUGE 2
postgresql.connections.maximum: database=main_db GAUGE 10

# Query metrics
postgresql.queries.total: database=main_db RATE 45.2
postgresql.queries.failed: database=main_db RATE 0.3
postgresql.query-time: database=main_db HIST [0.001, 0.005, ...]

# Transaction metrics
postgresql.transactions.total: database=main_db RATE 12.1
postgresql.transactions.committed: database=main_db RATE 11.8
postgresql.transactions.rolled-back: database=main_db RATE 0.3
```

#### MongoDB Metrics

```
# Connection metrics
mongo.connections.active: database=mongo_db GAUGE 2
mongo.connections.created: database=mongo_db RATE 0.1

# Operation metrics
mongo.queries.total: database=mongo_db, collection=users RATE 23.4
mongo.queries.failed: database=mongo_db, collection=users RATE 0.1
mongo.query-time: database=mongo_db, collection=users HIST [...]
```

#### Redis/Valkey Metrics

```
# Connection metrics
redis.connections.active: redis_name=main_redis GAUGE 1
redis.connections.created: redis_name=main_redis RATE 0.05

# Command metrics
redis.commands.total: redis_name=main_redis, command=GET RATE 156.7
redis.commands.failed: redis_name=main_redis, command=GET RATE 0.2
redis.command-time: redis_name=main_redis, command=GET HIST [...]
```

### gRPC Metrics

#### gRPC Server Metrics

```
# Request metrics
grpc.server.requests: grpc_service=UserService, grpc_method=GetUser, grpc_code=OK RATE 34.2
grpc.server.request-time: grpc_service=UserService, grpc_method=GetUser HIST [...]

# Connection metrics
grpc.server.connections.active: GAUGE 8
grpc.server.connections.opened: RATE 1.2
```

#### gRPC Client Metrics

```
# Client request metrics
grpc.client.requests: grpc_service=PaymentService, grpc_method=ProcessPayment, grpc_code=OK RATE 12.1
grpc.client.request-time: grpc_service=PaymentService, grpc_method=ProcessPayment HIST [...]
```

## Custom Metrics Implementation

### Adding Custom Metrics

```cpp
#include <userver/utils/statistics/writer.hpp>
#include <userver/utils/statistics/metric_tag.hpp>

class MyComponent final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "my-component";
    
    MyComponent(const components::ComponentConfig& config,
                const components::ComponentContext& context)
        : ComponentBase(config, context),
          custom_counter_(0),
          custom_histogram_() {
        
        // Register custom metrics
        auto& statistics_storage = context.FindComponent<components::StatisticsStorage>();
        statistics_holder_ = statistics_storage.GetStorage().RegisterWriter(
            "my-component",
            [this](utils::statistics::Writer& writer) {
                WriteStatistics(writer);
            }
        );
    }

private:
    void WriteStatistics(utils::statistics::Writer& writer) const {
        writer["custom_counter"] = custom_counter_.load();
        writer["custom_histogram"] = custom_histogram_;
        
        // Add labels/tags
        writer.ValueWithLabels(
            custom_requests_,
            {{"endpoint", "/api/v1/users"}, {"method", "GET"}}
        );
    }
    
    std::atomic<std::uint64_t> custom_counter_;
    utils::statistics::RecentPeriod<utils::statistics::Histogram> custom_histogram_;
    utils::statistics::RecentPeriod<utils::statistics::Rate> custom_requests_;
    utils::statistics::Entry statistics_holder_;
};
```

### Metric Tags and Labels

```cpp
// Using metric tags for automatic registration
class MetricsComponent {
private:
    USERVER_NAMESPACE::utils::statistics::MetricTag<USERVER_NAMESPACE::utils::statistics::Rate>
        requests_metric_{"my_service.requests"};
    
    USERVER_NAMESPACE::utils::statistics::MetricTag<USERVER_NAMESPACE::utils::statistics::Histogram>
        latency_metric_{"my_service.latency"};

public:
    void HandleRequest() {
        auto start_time = std::chrono::steady_clock::now();
        
        // Process request...
        
        requests_metric_.Account(1);
        
        auto duration = std::chrono::steady_clock::now() - start_time;
        latency_metric_.Account(
            std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
        );
    }
};
```

## Debug Monitoring Workflows

### 1. Performance Bottleneck Detection

```bash
# Monitor task processor queue buildup
curl -s 'http://localhost:8086/service/monitor?format=prometheus' | \
  grep 'engine_task_processors_tasks_queued' | \
  awk '{if($3 > 100) print "High queue: " $0}'

# Check HTTP handler response times
curl -s 'http://localhost:8086/service/monitor?format=prometheus' | \
  grep 'http_handler_request_time' | \
  grep 'quantile="0.99"'
```

### 2. Database Connection Issues

```bash
# Monitor PostgreSQL connection pool exhaustion
curl -s 'http://localhost:8086/service/monitor?format=prometheus' | \
  grep -E 'postgresql_connections_(active|maximum)' | \
  sort

# Check for database query failures
curl -s 'http://localhost:8086/service/monitor?format=prometheus' | \
  grep 'postgresql_queries_failed'
```

### 3. Memory and Resource Monitoring

```bash
# Monitor memory usage patterns
curl -s 'http://localhost:8086/service/monitor?format=prometheus' | \
  grep -E '(memory|heap)' | \
  head -20

# Check for resource leaks
watch -n 5 'curl -s "http://localhost:8086/service/monitor" | grep -E "(connections|handles)"'
```

### 4. Error Rate Analysis

```bash
# HTTP error rates
curl -s 'http://localhost:8086/service/monitor?format=prometheus' | \
  grep 'http_handler_requests' | \
  grep -E 'http_code="[45][0-9][0-9]"'

# Database error rates
curl -s 'http://localhost:8086/service/monitor?format=prometheus' | \
  grep -E '(queries_failed|transactions_rolled_back)'
```

## Monitoring Integration Patterns

### Prometheus Integration

```yaml
# prometheus.yml
scrape_configs:
  - job_name: 'userver-service'
    static_configs:
      - targets: ['localhost:8086']
    metrics_path: '/service/monitor'
    params:
      format: ['prometheus']
    scrape_interval: 15s
```

### Grafana Dashboard Queries

```promql
# Request rate
rate(http_handler_requests_total[5m])

# 99th percentile latency
histogram_quantile(0.99, rate(http_handler_request_time_bucket[5m]))

# Database connection utilization
postgresql_connections_active / postgresql_connections_maximum * 100

# Error rate
rate(http_handler_requests_total{http_code=~"5.."}[5m]) / 
rate(http_handler_requests_total[5m]) * 100
```

### Alerting Rules

```yaml
# alerting_rules.yml
groups:
  - name: userver_alerts
    rules:
      - alert: HighErrorRate
        expr: rate(http_handler_requests_total{http_code=~"5.."}[5m]) / rate(http_handler_requests_total[5m]) > 0.05
        for: 2m
        labels:
          severity: warning
        annotations:
          summary: "High error rate detected"
          
      - alert: DatabaseConnectionPoolExhaustion
        expr: postgresql_connections_active / postgresql_connections_maximum > 0.9
        for: 1m
        labels:
          severity: critical
        annotations:
          summary: "Database connection pool nearly exhausted"
```

## Debug-Specific Monitoring

### Development Metrics

```cpp
#ifdef DEBUG
class DebugMetrics {
private:
    utils::statistics::MetricTag<utils::statistics::Rate> debug_assertions_{"debug.assertions_failed"};
    utils::statistics::MetricTag<utils::statistics::Histogram> debug_timing_{"debug.operation_timing"};
    
public:
    void RecordAssertion() {
        debug_assertions_.Account(1);
    }
    
    void RecordTiming(std::chrono::milliseconds duration) {
        debug_timing_.Account(duration.count());
    }
};
#endif
```

### Runtime Debugging Metrics

```bash
# Monitor specific debug endpoints
curl 'http://localhost:8086/service/monitor' | grep -E '(debug|test|dev)'

# Check for memory leaks in debug builds
curl 'http://localhost:8086/service/monitor' | grep -E '(allocator|heap_profile)'
```

## Best Practices

### 1. Metric Naming Conventions

- Use hierarchical naming: `component.subcomponent.metric_name`
- Include units in names: `request_time_ms`, `memory_bytes`
- Use consistent labeling: `{service="user-service", version="1.2.3"}`

### 2. Performance Considerations

- Avoid high-cardinality labels in production
- Use sampling for expensive metrics
- Implement metric collection toggles for debug builds

### 3. Debugging Workflows

- Start with high-level metrics (request rates, error rates)
- Drill down to component-specific metrics
- Correlate metrics with logs and traces
- Use time-series analysis for trend detection

### 4. Monitoring Hygiene

- Regularly review and clean up unused metrics
- Document custom metrics and their purposes
- Implement metric validation in tests
- Use metric dashboards for visual debugging

## Integration with Other Debug Tools

### Correlation with Logging

```cpp
void ProcessRequest(const server::http::HttpRequest& request) {
    auto& span = tracing::Span::CurrentSpan();
    auto trace_id = span.GetTraceId();
    
    // Record metrics with trace correlation
    request_counter_.Account(1);
    
    LOG_INFO() << "Processing request"
               << logging::LogExtra{{"trace_id", trace_id}};
}
```

### Integration with Profiling

```bash
# Correlate metrics with profiling data
curl -s 'http://localhost:8086/service/monitor' > metrics_before.txt
# Run profiling session
curl -s 'http://localhost:8086/service/monitor' > metrics_after.txt
diff metrics_before.txt metrics_after.txt
```

This monitoring and metrics analysis framework provides comprehensive observability for debugging userver applications, enabling rapid identification and resolution of performance and reliability issues.