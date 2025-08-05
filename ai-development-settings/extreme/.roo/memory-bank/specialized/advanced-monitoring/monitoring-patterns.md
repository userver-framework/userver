# Advanced Monitoring Patterns in Userver

## Overview

Advanced monitoring is crucial for maintaining high-performance, reliable services. This document covers sophisticated monitoring patterns, observability strategies, and performance analysis techniques using the userver framework.

## Core Monitoring Concepts

### Metrics Collection Architecture

#### Multi-Dimensional Metrics
```cpp
#include <userver/utils/statistics/metrics_storage.hpp>
#include <userver/utils/statistics/histogram.hpp>

class AdvancedMetricsCollector {
public:
    struct MetricLabels {
        std::string service_name;
        std::string endpoint;
        std::string method;
        std::string status_code;
        std::string user_segment;
        std::string region;
    };
    
    AdvancedMetricsCollector(utils::statistics::MetricsStorage& storage)
        : storage_(storage) {
        InitializeMetrics();
    }
    
    void RecordRequest(const MetricLabels& labels,
                      std::chrono::milliseconds duration,
                      size_t request_size,
                      size_t response_size) {
        
        // Request duration histogram
        auto duration_key = BuildMetricKey("request_duration_ms", labels);
        storage_.GetHistogram(duration_key)->Account(duration.count());
        
        // Request rate counter
        auto rate_key = BuildMetricKey("requests_total", labels);
        storage_.GetCounter(rate_key)->Increment();
        
        // Request/Response size histograms
        auto req_size_key = BuildMetricKey("request_size_bytes", labels);
        storage_.GetHistogram(req_size_key)->Account(request_size);
        
        auto resp_size_key = BuildMetricKey("response_size_bytes", labels);
        storage_.GetHistogram(resp_size_key)->Account(response_size);
        
        // Business metrics
        RecordBusinessMetrics(labels, duration);
    }
    
    void RecordError(const MetricLabels& labels,
                    const std::string& error_type,
                    const std::string& error_code) {
        
        auto error_labels = labels;
        error_labels.status_code = error_code;
        
        auto error_key = BuildMetricKey("errors_total", error_labels);
        storage_.GetCounter(error_key)->Increment();
        
        // Error type breakdown
        auto type_key = BuildMetricKey("error_types_total", 
                                      {{"error_type", error_type}});
        storage_.GetCounter(type_key)->Increment();
    }
    
    void RecordResourceUsage(const std::string& resource_type,
                           double usage_percentage,
                           const MetricLabels& labels) {
        
        auto resource_labels = labels;
        resource_labels.service_name = resource_type;
        
        auto usage_key = BuildMetricKey("resource_usage_percent", resource_labels);
        storage_.GetGauge(usage_key)->Set(usage_percentage);
    }
    
private:
    void InitializeMetrics() {
        // Pre-create common metric families
        CreateHistogramFamily("request_duration_ms", 
                             "Request duration in milliseconds",
                             {1, 5, 10, 25, 50, 100, 250, 500, 1000, 2500, 5000});
        
        CreateHistogramFamily("request_size_bytes",
                             "Request size in bytes",
                             {100, 1000, 10000, 100000, 1000000});
        
        CreateCounterFamily("requests_total", "Total number of requests");
        CreateCounterFamily("errors_total", "Total number of errors");
        CreateGaugeFamily("resource_usage_percent", "Resource usage percentage");
    }
    
    std::string BuildMetricKey(const std::string& metric_name,
                              const MetricLabels& labels) {
        std::ostringstream key;
        key << metric_name
            << "{service=\"" << labels.service_name << "\""
            << ",endpoint=\"" << labels.endpoint << "\""
            << ",method=\"" << labels.method << "\""
            << ",status=\"" << labels.status_code << "\""
            << ",segment=\"" << labels.user_segment << "\""
            << ",region=\"" << labels.region << "\"}";
        return key.str();
    }
    
    void RecordBusinessMetrics(const MetricLabels& labels,
                              std::chrono::milliseconds duration) {
        // SLA compliance tracking
        if (duration > std::chrono::milliseconds(1000)) {
            auto sla_key = BuildMetricKey("sla_violations_total", labels);
            storage_.GetCounter(sla_key)->Increment();
        }
        
        // Performance tier classification
        std::string performance_tier;
        if (duration < std::chrono::milliseconds(100)) {
            performance_tier = "excellent";
        } else if (duration < std::chrono::milliseconds(500)) {
            performance_tier = "good";
        } else if (duration < std::chrono::milliseconds(1000)) {
            performance_tier = "acceptable";
        } else {
            performance_tier = "poor";
        }
        
        auto tier_labels = labels;
        tier_labels.user_segment = performance_tier;
        auto tier_key = BuildMetricKey("performance_tier_total", tier_labels);
        storage_.GetCounter(tier_key)->Increment();
    }
    
    void CreateHistogramFamily(const std::string& name,
                              const std::string& description,
                              const std::vector<double>& buckets) {
        // Implementation would register histogram family with storage
    }
    
    void CreateCounterFamily(const std::string& name,
                           const std::string& description) {
        // Implementation would register counter family with storage
    }
    
    void CreateGaugeFamily(const std::string& name,
                          const std::string& description) {
        // Implementation would register gauge family with storage
    }
    
    utils::statistics::MetricsStorage& storage_;
};
```

### Distributed Tracing

#### Advanced Trace Context
```cpp
#include <userver/tracing/span.hpp>
#include <userver/tracing/tracer.hpp>

class AdvancedTracer {
public:
    struct TraceContext {
        std::string trace_id;
        std::string span_id;
        std::string parent_span_id;
        std::map<std::string, std::string> baggage;
        std::chrono::system_clock::time_point start_time;
        std::string service_name;
        std::string operation_name;
    };
    
    class EnhancedSpan {
    public:
        EnhancedSpan(const std::string& operation_name,
                    tracing::Span parent_span = {})
            : span_(tracing::Span::CreateChild(parent_span, operation_name))
            , start_time_(std::chrono::steady_clock::now()) {
            
            // Add standard tags
            span_.AddTag("service.name", GetServiceName());
            span_.AddTag("service.version", GetServiceVersion());
            span_.AddTag("host.name", GetHostname());
            
            // Add correlation ID
            correlation_id_ = GenerateCorrelationId();
            span_.AddTag("correlation.id", correlation_id_);
        }
        
        ~EnhancedSpan() {
            auto duration = std::chrono::steady_clock::now() - start_time_;
            span_.AddTag("duration.ms", 
                        std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
        }
        
        void AddBusinessContext(const std::string& user_id,
                               const std::string& tenant_id,
                               const std::string& feature_flag = "") {
            span_.AddTag("user.id", user_id);
            span_.AddTag("tenant.id", tenant_id);
            if (!feature_flag.empty()) {
                span_.AddTag("feature.flag", feature_flag);
            }
        }
        
        void AddDatabaseContext(const std::string& db_type,
                               const std::string& db_name,
                               const std::string& query_type) {
            span_.AddTag("db.type", db_type);
            span_.AddTag("db.name", db_name);
            span_.AddTag("db.operation", query_type);
        }
        
        void AddHttpContext(const std::string& method,
                           const std::string& url,
                           int status_code,
                           size_t request_size = 0,
                           size_t response_size = 0) {
            span_.AddTag("http.method", method);
            span_.AddTag("http.url", url);
            span_.AddTag("http.status_code", std::to_string(status_code));
            
            if (request_size > 0) {
                span_.AddTag("http.request.size", std::to_string(request_size));
            }
            if (response_size > 0) {
                span_.AddTag("http.response.size", std::to_string(response_size));
            }
        }
        
        void AddErrorContext(const std::exception& ex,
                           const std::string& error_code = "",
                           bool is_retryable = false) {
            span_.AddTag("error", "true");
            span_.AddTag("error.message", ex.what());
            span_.AddTag("error.type", typeid(ex).name());
            
            if (!error_code.empty()) {
                span_.AddTag("error.code", error_code);
            }
            
            span_.AddTag("error.retryable", is_retryable ? "true" : "false");
        }
        
        void AddCustomMetrics(const std::map<std::string, std::string>& metrics) {
            for (const auto& [key, value] : metrics) {
                span_.AddTag("metric." + key, value);
            }
        }
        
        std::string GetCorrelationId() const {
            return correlation_id_;
        }
        
        tracing::Span& GetSpan() { return span_; }
        
    private:
        std::string GenerateCorrelationId() {
            // Generate unique correlation ID
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_int_distribution<> dis(0, 15);
            
            std::string id;
            for (int i = 0; i < 16; ++i) {
                id += "0123456789abcdef"[dis(gen)];
            }
            return id;
        }
        
        std::string GetServiceName() {
            // Implementation would get from configuration
            return "my-service";
        }
        
        std::string GetServiceVersion() {
            // Implementation would get from build info
            return "1.0.0";
        }
        
        std::string GetHostname() {
            char hostname[256];
            gethostname(hostname, sizeof(hostname));
            return std::string(hostname);
        }
        
        tracing::Span span_;
        std::chrono::steady_clock::time_point start_time_;
        std::string correlation_id_;
    };
    
    static EnhancedSpan CreateSpan(const std::string& operation_name) {
        return EnhancedSpan(operation_name);
    }
    
    static EnhancedSpan CreateChildSpan(const std::string& operation_name,
                                       const EnhancedSpan& parent) {
        return EnhancedSpan(operation_name, parent.GetSpan());
    }
};
```

### Performance Monitoring

#### Real-time Performance Analytics
```cpp
class PerformanceAnalyzer {
public:
    struct PerformanceMetrics {
        std::chrono::milliseconds p50_latency{0};
        std::chrono::milliseconds p95_latency{0};
        std::chrono::milliseconds p99_latency{0};
        double requests_per_second{0.0};
        double error_rate{0.0};
        double cpu_usage{0.0};
        double memory_usage{0.0};
        size_t active_connections{0};
        size_t queue_depth{0};
    };
    
    struct PerformanceAlert {
        enum class Severity { kInfo, kWarning, kCritical };
        
        Severity severity;
        std::string metric_name;
        double current_value;
        double threshold;
        std::string description;
        std::chrono::system_clock::time_point timestamp;
    };
    
    PerformanceAnalyzer(utils::statistics::MetricsStorage& storage)
        : storage_(storage) {
        InitializeThresholds();
        StartAnalysis();
    }
    
    void RecordLatency(const std::string& operation,
                      std::chrono::milliseconds latency) {
        std::lock_guard lock(latency_mutex_);
        latency_samples_[operation].push_back({
            latency,
            std::chrono::steady_clock::now()
        });
        
        // Keep only recent samples (last 5 minutes)
        CleanOldSamples(operation);
    }
    
    PerformanceMetrics GetCurrentMetrics(const std::string& operation) {
        std::lock_guard lock(latency_mutex_);
        
        PerformanceMetrics metrics;
        
        if (latency_samples_[operation].empty()) {
            return metrics;
        }
        
        // Calculate percentiles
        auto& samples = latency_samples_[operation];
        std::vector<std::chrono::milliseconds> latencies;
        
        for (const auto& sample : samples) {
            latencies.push_back(sample.latency);
        }
        
        std::sort(latencies.begin(), latencies.end());
        
        size_t p50_idx = latencies.size() * 0.5;
        size_t p95_idx = latencies.size() * 0.95;
        size_t p99_idx = latencies.size() * 0.99;
        
        metrics.p50_latency = latencies[p50_idx];
        metrics.p95_latency = latencies[p95_idx];
        metrics.p99_latency = latencies[p99_idx];
        
        // Calculate RPS
        auto now = std::chrono::steady_clock::now();
        auto one_second_ago = now - std::chrono::seconds(1);
        
        size_t recent_requests = std::count_if(samples.begin(), samples.end(),
            [one_second_ago](const LatencySample& sample) {
                return sample.timestamp >= one_second_ago;
            });
        
        metrics.requests_per_second = static_cast<double>(recent_requests);
        
        // Get other metrics from storage
        metrics.error_rate = GetErrorRate(operation);
        metrics.cpu_usage = GetCpuUsage();
        metrics.memory_usage = GetMemoryUsage();
        metrics.active_connections = GetActiveConnections();
        metrics.queue_depth = GetQueueDepth();
        
        return metrics;
    }
    
    std::vector<PerformanceAlert> CheckAlerts(const std::string& operation) {
        std::vector<PerformanceAlert> alerts;
        auto metrics = GetCurrentMetrics(operation);
        
        // Check latency thresholds
        if (metrics.p95_latency > thresholds_.max_p95_latency) {
            alerts.push_back({
                PerformanceAlert::Severity::kWarning,
                "p95_latency",
                static_cast<double>(metrics.p95_latency.count()),
                static_cast<double>(thresholds_.max_p95_latency.count()),
                "P95 latency exceeded threshold",
                std::chrono::system_clock::now()
            });
        }
        
        if (metrics.p99_latency > thresholds_.max_p99_latency) {
            alerts.push_back({
                PerformanceAlert::Severity::kCritical,
                "p99_latency",
                static_cast<double>(metrics.p99_latency.count()),
                static_cast<double>(thresholds_.max_p99_latency.count()),
                "P99 latency exceeded critical threshold",
                std::chrono::system_clock::now()
            });
        }
        
        // Check error rate
        if (metrics.error_rate > thresholds_.max_error_rate) {
            auto severity = metrics.error_rate > thresholds_.critical_error_rate ?
                           PerformanceAlert::Severity::kCritical :
                           PerformanceAlert::Severity::kWarning;
            
            alerts.push_back({
                severity,
                "error_rate",
                metrics.error_rate,
                thresholds_.max_error_rate,
                "Error rate exceeded threshold",
                std::chrono::system_clock::now()
            });
        }
        
        // Check resource usage
        if (metrics.cpu_usage > thresholds_.max_cpu_usage) {
            alerts.push_back({
                PerformanceAlert::Severity::kWarning,
                "cpu_usage",
                metrics.cpu_usage,
                thresholds_.max_cpu_usage,
                "CPU usage exceeded threshold",
                std::chrono::system_clock::now()
            });
        }
        
        if (metrics.memory_usage > thresholds_.max_memory_usage) {
            alerts.push_back({
                PerformanceAlert::Severity::kCritical,
                "memory_usage",
                metrics.memory_usage,
                thresholds_.max_memory_usage,
                "Memory usage exceeded critical threshold",
                std::chrono::system_clock::now()
            });
        }
        
        return alerts;
    }
    
private:
    struct LatencySample {
        std::chrono::milliseconds latency;
        std::chrono::steady_clock::time_point timestamp;
    };
    
    struct PerformanceThresholds {
        std::chrono::milliseconds max_p95_latency{std::chrono::milliseconds(1000)};
        std::chrono::milliseconds max_p99_latency{std::chrono::milliseconds(2000)};
        double max_error_rate{0.05}; // 5%
        double critical_error_rate{0.10}; // 10%
        double max_cpu_usage{0.80}; // 80%
        double max_memory_usage{0.85}; // 85%
    };
    
    void InitializeThresholds() {
        // Load thresholds from configuration
        // This is a simplified example
    }
    
    void StartAnalysis() {
        analysis_task_ = engine::AsyncNoSpan([this]() {
            while (!stop_analysis_) {
                PerformAnalysis();
                engine::SleepFor(std::chrono::seconds(10));
            }
        });
    }
    
    void PerformAnalysis() {
        // Analyze trends, detect anomalies, generate insights
        for (const auto& [operation, samples] : latency_samples_) {
            auto alerts = CheckAlerts(operation);
            for (const auto& alert : alerts) {
                HandleAlert(alert);
            }
        }
    }
    
    void HandleAlert(const PerformanceAlert& alert) {
        LOG_WARNING() << "Performance alert: " << alert.description
                     << " (current: " << alert.current_value
                     << ", threshold: " << alert.threshold << ")";
        
        // Send to alerting system
        // Store in alert history
        // Trigger automated responses if configured
    }
    
    void CleanOldSamples(const std::string& operation) {
        auto& samples = latency_samples_[operation];
        auto cutoff = std::chrono::steady_clock::now() - std::chrono::minutes(5);
        
        samples.erase(
            std::remove_if(samples.begin(), samples.end(),
                          [cutoff](const LatencySample& sample) {
                              return sample.timestamp < cutoff;
                          }),
            samples.end());
    }
    
    double GetErrorRate(const std::string& operation) {
        // Implementation would calculate from metrics storage
        return 0.0;
    }
    
    double GetCpuUsage() {
        // Implementation would read system metrics
        return 0.0;
    }
    
    double GetMemoryUsage() {
        // Implementation would read system metrics
        return 0.0;
    }
    
    size_t GetActiveConnections() {
        // Implementation would query connection pools
        return 0;
    }
    
    size_t GetQueueDepth() {
        // Implementation would query task queues
        return 0;
    }
    
    utils::statistics::MetricsStorage& storage_;
    PerformanceThresholds thresholds_;
    
    std::mutex latency_mutex_;
    std::map<std::string, std::vector<LatencySample>> latency_samples_;
    
    engine::TaskWithResult<void> analysis_task_;
    std::atomic<bool> stop_analysis_{false};
};
```

## Application Performance Monitoring (APM)

### Business Transaction Monitoring
```cpp
class BusinessTransactionMonitor {
public:
    struct TransactionContext {
        std::string transaction_id;
        std::string transaction_type;
        std::string user_id;
        std::string session_id;
        std::chrono::system_clock::time_point start_time;
        std::map<std::string, std::string> business_attributes;
    };
    
    struct TransactionStep {
        std::string step_name;
        std::chrono::system_clock::time_point start_time;
        std::chrono::system_clock::time_point end_time;
        bool success;
        std::string error_message;
        std::map<std::string, std::string> step_attributes;
    };
    
    struct TransactionResult {
        TransactionContext context;
        std::vector<TransactionStep> steps;
        std::chrono::milliseconds total_duration;
        bool success;
        std::string failure_reason;
    };
    
    class TransactionTracker {
    public:
        TransactionTracker(const std::string& transaction_type,
                          const std::string& user_id,
                          BusinessTransactionMonitor& monitor)
            : monitor_(monitor) {
            
            context_.transaction_id = GenerateTransactionId();
            context_.transaction_type = transaction_type;
            context_.user_id = user_id;
            context_.start_time = std::chrono::system_clock::now();
        }
        
        ~TransactionTracker() {
            if (!finalized_) {
                Finalize(false, "Transaction not properly finalized");
            }
        }
        
        void AddBusinessAttribute(const std::string& key, const std::string& value) {
            context_.business_attributes[key] = value;
        }
        
        void StartStep(const std::string& step_name) {
            current_step_ = TransactionStep{
                step_name,
                std::chrono::system_clock::now(),
                {},
                false,
                "",
                {}
            };
        }
        
        void EndStep(bool success, const std::string& error_message = "") {
            if (current_step_.step_name.empty()) {
                LOG_WARNING() << "EndStep called without StartStep";
                return;
            }
            
            current_step_.end_time = std::chrono::system_clock::now();
            current_step_.success = success;
            current_step_.error_message = error_message;
            
            steps_.push_back(current_step_);
            current_step_ = {}; // Reset
        }
        
        void AddStepAttribute(const std::string& key, const std::string& value) {
            current_step_.step_attributes[key] = value;
        }
        
        void Finalize(bool success, const std::string& failure_reason = "") {
            if (finalized_) return;
            
            auto end_time = std::chrono::system_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                end_time - context_.start_time);
            
            TransactionResult result{
                context_,
                steps_,
                duration,
                success,
                failure_reason
            };
            
            monitor_.RecordTransaction(result);
            finalized_ = true;
        }
        
        std::string GetTransactionId() const {
            return context_.transaction_id;
        }
        
    private:
        std::string GenerateTransactionId() {
            // Generate unique transaction ID
            static std::atomic<uint64_t> counter{0};
            auto now = std::chrono::system_clock::now();
            auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()).count();
            
            return std::to_string(timestamp) + "-" + std::to_string(counter++);
        }
        
        BusinessTransactionMonitor& monitor_;
        TransactionContext context_;
        std::vector<TransactionStep> steps_;
        TransactionStep current_step_;
        bool finalized_{false};
    };
    
    std::unique_ptr<TransactionTracker> StartTransaction(
        const std::string& transaction_type,
        const std::string& user_id) {
        
        return std::make_unique<TransactionTracker>(transaction_type, user_id, *this);
    }
    
    void RecordTransaction(const TransactionResult& result) {
        std::lock_guard lock(transactions_mutex_);
        
        // Store transaction result
        transaction_history_.push_back(result);
        
        // Update aggregated metrics
        UpdateAggregatedMetrics(result);
        
        // Check for anomalies
        CheckTransactionAnomalies(result);
        
        // Cleanup old transactions
        CleanupOldTransactions();
    }
    
    struct TransactionMetrics {
        size_t total_transactions{0};
        size_t successful_transactions{0};
        size_t failed_transactions{0};
        std::chrono::milliseconds avg_duration{0};
        std::chrono::milliseconds p95_duration{0};
        std::chrono::milliseconds p99_duration{0};
        double success_rate{0.0};
        std::map<std::string, size_t> failure_reasons;
        std::map<std::string, std::chrono::milliseconds> step_durations;
    };
    
    TransactionMetrics GetMetrics(const std::string& transaction_type,
                                 std::chrono::minutes time_window = std::chrono::minutes(60)) {
        std::lock_guard lock(transactions_mutex_);
        
        TransactionMetrics metrics;
        auto cutoff = std::chrono::system_clock::now() - time_window;
        
        std::vector<std::chrono::milliseconds> durations;
        
        for (const auto& transaction : transaction_history_) {
            if (transaction.context.transaction_type != transaction_type ||
                transaction.context.start_time < cutoff) {
                continue;
            }
            
            metrics.total_transactions++;
            durations.push_back(transaction.total_duration);
            
            if (transaction.success) {
                metrics.successful_transactions++;
            } else {
                metrics.failed_transactions++;
                metrics.failure_reasons[transaction.failure_reason]++;
            }
            
            // Aggregate step durations
            for (const auto& step : transaction.steps) {
                auto step_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    step.end_time - step.start_time);
                metrics.step_durations[step.step_name] += step_duration;
            }
        }
        
        if (!durations.empty()) {
            // Calculate percentiles
            std::sort(durations.begin(), durations.end());
            
            auto total_duration = std::accumulate(durations.begin(), durations.end(),
                                                std::chrono::milliseconds(0));
            metrics.avg_duration = total_duration / durations.size();
            
            size_t p95_idx = durations.size() * 0.95;
            size_t p99_idx = durations.size() * 0.99;
            
            metrics.p95_duration = durations[p95_idx];
            metrics.p99_duration = durations[p99_idx];
            
            metrics.success_rate = static_cast<double>(metrics.successful_transactions) /
                                  metrics.total_transactions;
        }
        
        return metrics;
    }
    
private:
    void UpdateAggregatedMetrics(const TransactionResult& result) {
        // Update real-time metrics for dashboards
        auto& type_metrics = aggregated_metrics_[result.context.transaction_type];
        
        type_metrics.total_count++;
        type_metrics.total_duration += result.total_duration;
        
        if (result.success) {
            type_metrics.success_count++;
        } else {
            type_metrics.failure_count++;
        }
    }
    
    void CheckTransactionAnomalies(const TransactionResult& result) {
        // Simple anomaly detection
        auto metrics = GetMetrics(result.context.transaction_type, std::chrono::minutes(10));
        
        // Check if this transaction is significantly slower than average
        if (result.total_duration > metrics.avg_duration * 3) {
            LOG_WARNING() << "Slow transaction detected: " << result.context.transaction_id
                         << " took " << result.total_duration.count() << "ms"
                         << " (avg: " << metrics.avg_duration.count() << "ms)";
        }
        
        // Check for unusual failure patterns
        if (!result.success && metrics.success_rate > 0.95) {
            LOG_WARNING() << "Transaction failure in normally stable operation: "
                         << result.context.transaction_id
                         << " reason: " << result.failure_reason;
        }
    }
    
    void CleanupOldTransactions() {
        auto cutoff = std::chrono::system_clock::now() - std::chrono::hours(24);
        
        transaction_history_.erase(
            std::remove_if(transaction_history_.begin(), transaction_history_.end(),
                          [cutoff](const TransactionResult& transaction) {
                              return transaction.context.start_time < cutoff;
                          }),
            transaction_history_.end());
    }
    
    struct AggregatedMetrics {
        size_t total_count{0};
        size_t success_count{0};
        size_t failure_count{0};
        std::chrono::milliseconds total_duration{0};
    };
    
    std::mutex transactions_mutex_;
    std::vector<TransactionResult> transaction_history_;
    std::map<std::string, AggregatedMetrics> aggregated_metrics_;
};
```

## Health Check and Readiness Probes

### Comprehensive Health Monitoring
```cpp
class HealthCheckSystem {
public:
    enum class HealthStatus {
        kHealthy,
        kDegraded,
        kUnhealthy
    };
    
    struct ComponentHealth {
        std::string component_name;
        HealthStatus status;
        std::string message;
        std::chrono::system_clock::time_point last_check;
        std::chrono::milliseconds response_time;
        std::map<std::string, std::string> details;
    };
    
    struct SystemHealth {
        HealthStatus overall_status;
        std::vector<ComponentHealth> components;
        std::chrono::system_clock::time_point timestamp;
        std::string version;
        std::chrono