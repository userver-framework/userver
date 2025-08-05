# Chaos Testing Patterns in Userver

## Overview

Chaos testing is a discipline in software engineering that experiments on a distributed system to build confidence in the system's capability to withstand turbulent conditions in production. This document covers chaos testing patterns and implementations using the userver framework.

## Core Chaos Testing Concepts

### Chaos Engineering Principles

#### Hypothesis-Driven Testing
```cpp
class ChaosExperiment {
public:
    struct Hypothesis {
        std::string description;
        std::function<bool()> steady_state_validator;
        std::chrono::milliseconds duration;
        double confidence_threshold{0.95};
    };
    
    struct ExperimentResult {
        bool hypothesis_validated;
        std::vector<std::string> observations;
        std::chrono::milliseconds actual_duration;
        double confidence_score;
    };
    
    ChaosExperiment(const Hypothesis& hypothesis)
        : hypothesis_(hypothesis) {}
    
    ExperimentResult RunExperiment() {
        ExperimentResult result;
        auto start_time = std::chrono::steady_clock::now();
        
        // Establish steady state
        if (!ValidateSteadyState()) {
            result.observations.push_back("Failed to establish steady state");
            return result;
        }
        
        // Introduce chaos
        auto chaos_task = engine::AsyncNoSpan([this]() {
            InjectChaos();
        });
        
        // Monitor system behavior
        auto monitor_task = engine::AsyncNoSpan([this, &result]() {
            MonitorSystemBehavior(result);
        });
        
        // Wait for experiment duration
        engine::SleepFor(hypothesis_.duration);
        
        // Stop chaos injection
        StopChaos();
        
        // Validate hypothesis
        result.hypothesis_validated = ValidateSteadyState();
        result.actual_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time);
        
        return result;
    }
    
private:
    bool ValidateSteadyState() {
        return hypothesis_.steady_state_validator();
    }
    
    virtual void InjectChaos() = 0;
    virtual void StopChaos() = 0;
    virtual void MonitorSystemBehavior(ExperimentResult& result) = 0;
    
    Hypothesis hypothesis_;
};
```

### Fault Injection Patterns

#### Network Chaos
```cpp
class NetworkChaosInjector {
public:
    enum class FaultType {
        kLatency,
        kPacketLoss,
        kBandwidthLimit,
        kConnectionDrop,
        kDnsFailure
    };
    
    struct NetworkFault {
        FaultType type;
        std::chrono::milliseconds duration;
        double intensity; // 0.0 to 1.0
        std::string target_host;
        int target_port{0};
    };
    
    NetworkChaosInjector(clients::http::Client& http_client)
        : http_client_(http_client) {}
    
    void InjectFault(const NetworkFault& fault) {
        switch (fault.type) {
            case FaultType::kLatency:
                InjectLatency(fault);
                break;
            case FaultType::kPacketLoss:
                InjectPacketLoss(fault);
                break;
            case FaultType::kBandwidthLimit:
                InjectBandwidthLimit(fault);
                break;
            case FaultType::kConnectionDrop:
                InjectConnectionDrop(fault);
                break;
            case FaultType::kDnsFailure:
                InjectDnsFailure(fault);
                break;
        }
        
        // Schedule fault removal
        engine::AsyncNoSpan([this, fault]() {
            engine::SleepFor(fault.duration);
            RemoveFault(fault);
        });
    }
    
private:
    void InjectLatency(const NetworkFault& fault) {
        // Implement latency injection using traffic control or proxy
        auto delay = std::chrono::milliseconds(
            static_cast<int>(fault.intensity * 1000)); // Max 1 second delay
        
        latency_injector_ = std::make_unique<LatencyInjector>(
            fault.target_host, fault.target_port, delay);
        latency_injector_->Start();
    }
    
    void InjectPacketLoss(const NetworkFault& fault) {
        // Implement packet loss using network simulation
        auto loss_rate = fault.intensity; // 0.0 to 1.0
        
        packet_loss_injector_ = std::make_unique<PacketLossInjector>(
            fault.target_host, fault.target_port, loss_rate);
        packet_loss_injector_->Start();
    }
    
    void InjectBandwidthLimit(const NetworkFault& fault) {
        // Implement bandwidth limiting
        auto bandwidth_limit = static_cast<size_t>(
            fault.intensity * 1024 * 1024); // MB/s
        
        bandwidth_limiter_ = std::make_unique<BandwidthLimiter>(
            fault.target_host, fault.target_port, bandwidth_limit);
        bandwidth_limiter_->Start();
    }
    
    void InjectConnectionDrop(const NetworkFault& fault) {
        // Implement connection dropping
        connection_dropper_ = std::make_unique<ConnectionDropper>(
            fault.target_host, fault.target_port, fault.intensity);
        connection_dropper_->Start();
    }
    
    void InjectDnsFailure(const NetworkFault& fault) {
        // Implement DNS failure simulation
        dns_chaos_ = std::make_unique<DnsChaosInjector>(
            fault.target_host, fault.intensity);
        dns_chaos_->Start();
    }
    
    void RemoveFault(const NetworkFault& fault) {
        // Clean up fault injectors
        latency_injector_.reset();
        packet_loss_injector_.reset();
        bandwidth_limiter_.reset();
        connection_dropper_.reset();
        dns_chaos_.reset();
    }
    
    clients::http::Client& http_client_;
    
    // Fault injectors
    std::unique_ptr<LatencyInjector> latency_injector_;
    std::unique_ptr<PacketLossInjector> packet_loss_injector_;
    std::unique_ptr<BandwidthLimiter> bandwidth_limiter_;
    std::unique_ptr<ConnectionDropper> connection_dropper_;
    std::unique_ptr<DnsChaosInjector> dns_chaos_;
};
```

#### Resource Chaos
```cpp
class ResourceChaosInjector {
public:
    enum class ResourceType {
        kCpu,
        kMemory,
        kDisk,
        kFileDescriptors
    };
    
    struct ResourceFault {
        ResourceType type;
        std::chrono::milliseconds duration;
        double intensity; // Resource consumption level (0.0 to 1.0)
    };
    
    void InjectResourceStress(const ResourceFault& fault) {
        switch (fault.type) {
            case ResourceType::kCpu:
                InjectCpuStress(fault);
                break;
            case ResourceType::kMemory:
                InjectMemoryStress(fault);
                break;
            case ResourceType::kDisk:
                InjectDiskStress(fault);
                break;
            case ResourceType::kFileDescriptors:
                InjectFdStress(fault);
                break;
        }
    }
    
private:
    void InjectCpuStress(const ResourceFault& fault) {
        auto cpu_cores = std::thread::hardware_concurrency();
        auto stress_threads = static_cast<size_t>(cpu_cores * fault.intensity);
        
        for (size_t i = 0; i < stress_threads; ++i) {
            cpu_stress_tasks_.push_back(engine::AsyncNoSpan([fault]() {
                auto end_time = std::chrono::steady_clock::now() + fault.duration;
                
                while (std::chrono::steady_clock::now() < end_time) {
                    // Busy loop to consume CPU
                    volatile int dummy = 0;
                    for (int j = 0; j < 1000000; ++j) {
                        dummy += j;
                    }
                    
                    // Yield occasionally to prevent complete system freeze
                    if (dummy % 10000 == 0) {
                        engine::Yield();
                    }
                }
            }));
        }
    }
    
    void InjectMemoryStress(const ResourceFault& fault) {
        memory_stress_task_ = engine::AsyncNoSpan([fault]() {
            std::vector<std::vector<char>> memory_hogs;
            
            // Allocate memory based on intensity
            auto total_memory = GetAvailableMemory();
            auto target_allocation = static_cast<size_t>(
                total_memory * fault.intensity);
            
            const size_t chunk_size = 1024 * 1024; // 1MB chunks
            auto chunks_needed = target_allocation / chunk_size;
            
            try {
                for (size_t i = 0; i < chunks_needed; ++i) {
                    memory_hogs.emplace_back(chunk_size, 'X');
                    
                    // Touch the memory to ensure it's actually allocated
                    std::fill(memory_hogs.back().begin(), 
                             memory_hogs.back().end(), 
                             static_cast<char>(i % 256));
                    
                    engine::Yield();
                }
                
                // Hold memory for duration
                engine::SleepFor(fault.duration);
                
            } catch (const std::bad_alloc& ex) {
                LOG_WARNING() << "Memory allocation failed during chaos test: " 
                             << ex.what();
            }
            
            // Memory is automatically freed when vector goes out of scope
        });
    }
    
    void InjectDiskStress(const ResourceFault& fault) {
        disk_stress_task_ = engine::AsyncNoSpan([fault]() {
            const std::string temp_file = "/tmp/chaos_disk_stress.tmp";
            const size_t write_size = 1024 * 1024; // 1MB
            
            auto end_time = std::chrono::steady_clock::now() + fault.duration;
            
            while (std::chrono::steady_clock::now() < end_time) {
                try {
                    std::ofstream file(temp_file, std::ios::binary | std::ios::app);
                    std::vector<char> data(write_size, 'D');
                    
                    file.write(data.data(), data.size());
                    file.flush();
                    file.close();
                    
                    // Control write intensity
                    auto sleep_duration = std::chrono::milliseconds(
                        static_cast<int>((1.0 - fault.intensity) * 100));
                    engine::SleepFor(sleep_duration);
                    
                } catch (const std::exception& ex) {
                    LOG_WARNING() << "Disk stress injection failed: " << ex.what();
                    break;
                }
            }
            
            // Cleanup
            std::remove(temp_file.c_str());
        });
    }
    
    void InjectFdStress(const ResourceFault& fault) {
        fd_stress_task_ = engine::AsyncNoSpan([fault]() {
            std::vector<int> file_descriptors;
            
            // Open many file descriptors
            auto max_fds = static_cast<size_t>(getrlimit_nofile() * fault.intensity);
            
            for (size_t i = 0; i < max_fds; ++i) {
                int fd = open("/dev/null", O_RDONLY);
                if (fd != -1) {
                    file_descriptors.push_back(fd);
                } else {
                    break; // Can't open more FDs
                }
                
                if (i % 100 == 0) {
                    engine::Yield();
                }
            }
            
            // Hold FDs for duration
            engine::SleepFor(fault.duration);
            
            // Cleanup
            for (int fd : file_descriptors) {
                close(fd);
            }
        });
    }
    
    size_t GetAvailableMemory() {
        // Simplified - in practice, read from /proc/meminfo
        return 1024 * 1024 * 1024; // 1GB
    }
    
    int getrlimit_nofile() {
        struct rlimit limit;
        if (getrlimit(RLIMIT_NOFILE, &limit) == 0) {
            return static_cast<int>(limit.rlim_cur);
        }
        return 1024; // Default fallback
    }
    
    std::vector<engine::TaskWithResult<void>> cpu_stress_tasks_;
    engine::TaskWithResult<void> memory_stress_task_;
    engine::TaskWithResult<void> disk_stress_task_;
    engine::TaskWithResult<void> fd_stress_task_;
};
```

## Service-Level Chaos Testing

### Database Chaos
```cpp
class DatabaseChaosInjector {
public:
    enum class DatabaseFaultType {
        kConnectionFailure,
        kSlowQueries,
        kTransactionFailure,
        kDataCorruption,
        kReplicationLag
    };
    
    struct DatabaseFault {
        DatabaseFaultType type;
        std::chrono::milliseconds duration;
        double probability; // Fault injection probability (0.0 to 1.0)
        std::string target_table;
    };
    
    DatabaseChaosInjector(storages::postgres::ClusterPtr cluster)
        : cluster_(cluster) {}
    
    void InjectFault(const DatabaseFault& fault) {
        switch (fault.type) {
            case DatabaseFaultType::kConnectionFailure:
                InjectConnectionFailure(fault);
                break;
            case DatabaseFaultType::kSlowQueries:
                InjectSlowQueries(fault);
                break;
            case DatabaseFaultType::kTransactionFailure:
                InjectTransactionFailure(fault);
                break;
            case DatabaseFaultType::kDataCorruption:
                InjectDataCorruption(fault);
                break;
            case DatabaseFaultType::kReplicationLag:
                InjectReplicationLag(fault);
                break;
        }
    }
    
private:
    void InjectConnectionFailure(const DatabaseFault& fault) {
        connection_chaos_active_ = true;
        
        // Intercept database connections
        original_execute_ = cluster_->GetExecuteFunction();
        
        cluster_->SetExecuteFunction([this, fault](auto&&... args) {
            if (ShouldInjectFault(fault.probability)) {
                throw storages::postgres::ConnectionError("Chaos: Connection failed");
            }
            return original_execute_(std::forward<decltype(args)>(args)...);
        });
        
        // Schedule fault removal
        engine::AsyncNoSpan([this, fault]() {
            engine::SleepFor(fault.duration);
            RemoveConnectionFailure();
        });
    }
    
    void InjectSlowQueries(const DatabaseFault& fault) {
        slow_query_chaos_active_ = true;
        
        cluster_->SetExecuteFunction([this, fault](auto&&... args) {
            if (ShouldInjectFault(fault.probability)) {
                // Add artificial delay
                auto delay = std::chrono::milliseconds(
                    static_cast<int>(fault.probability * 5000)); // Up to 5 seconds
                engine::SleepFor(delay);
            }
            return original_execute_(std::forward<decltype(args)>(args)...);
        });
        
        engine::AsyncNoSpan([this, fault]() {
            engine::SleepFor(fault.duration);
            RemoveSlowQueries();
        });
    }
    
    void InjectTransactionFailure(const DatabaseFault& fault) {
        transaction_chaos_active_ = true;
        
        cluster_->SetTransactionFunction([this, fault](auto&&... args) {
            if (ShouldInjectFault(fault.probability)) {
                throw storages::postgres::TransactionError("Chaos: Transaction failed");
            }
            return original_transaction_(std::forward<decltype(args)>(args)...);
        });
        
        engine::AsyncNoSpan([this, fault]() {
            engine::SleepFor(fault.duration);
            RemoveTransactionFailure();
        });
    }
    
    bool ShouldInjectFault(double probability) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<> dis(0.0, 1.0);
        
        return dis(gen) < probability;
    }
    
    void RemoveConnectionFailure() {
        connection_chaos_active_ = false;
        cluster_->SetExecuteFunction(original_execute_);
    }
    
    void RemoveSlowQueries() {
        slow_query_chaos_active_ = false;
        cluster_->SetExecuteFunction(original_execute_);
    }
    
    void RemoveTransactionFailure() {
        transaction_chaos_active_ = false;
        cluster_->SetTransactionFunction(original_transaction_);
    }
    
    storages::postgres::ClusterPtr cluster_;
    
    // Chaos state
    std::atomic<bool> connection_chaos_active_{false};
    std::atomic<bool> slow_query_chaos_active_{false};
    std::atomic<bool> transaction_chaos_active_{false};
    
    // Original functions
    std::function<void()> original_execute_;
    std::function<void()> original_transaction_;
};
```

### HTTP Client Chaos
```cpp
class HttpClientChaosInjector {
public:
    enum class HttpFaultType {
        kTimeout,
        kServerError,
        kClientError,
        kNetworkError,
        kSlowResponse
    };
    
    struct HttpFault {
        HttpFaultType type;
        std::chrono::milliseconds duration;
        double probability;
        std::string target_url_pattern;
        int error_code{500};
    };
    
    HttpClientChaosInjector(clients::http::Client& http_client)
        : http_client_(http_client) {}
    
    void InjectFault(const HttpFault& fault) {
        active_faults_.push_back(fault);
        
        // Install request interceptor
        if (!interceptor_installed_) {
            InstallInterceptor();
            interceptor_installed_ = true;
        }
        
        // Schedule fault removal
        engine::AsyncNoSpan([this, fault]() {
            engine::SleepFor(fault.duration);
            RemoveFault(fault);
        });
    }
    
private:
    void InstallInterceptor() {
        http_client_.SetRequestInterceptor([this](auto& request) {
            for (const auto& fault : active_faults_) {
                if (ShouldApplyFault(request, fault)) {
                    ApplyFault(request, fault);
                    break; // Apply only one fault per request
                }
            }
        });
    }
    
    bool ShouldApplyFault(const clients::http::Request& request,
                         const HttpFault& fault) {
        // Check URL pattern
        if (!fault.target_url_pattern.empty()) {
            if (request.GetUrl().find(fault.target_url_pattern) == std::string::npos) {
                return false;
            }
        }
        
        // Check probability
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<> dis(0.0, 1.0);
        
        return dis(gen) < fault.probability;
    }
    
    void ApplyFault(clients::http::Request& request, const HttpFault& fault) {
        switch (fault.type) {
            case HttpFaultType::kTimeout:
                request.timeout(std::chrono::milliseconds(1)); // Very short timeout
                break;
                
            case HttpFaultType::kServerError:
                // Modify request to trigger server error
                request.header("X-Chaos-Inject", "server-error");
                break;
                
            case HttpFaultType::kClientError:
                // Modify request to trigger client error
                request.header("X-Chaos-Inject", "client-error");
                break;
                
            case HttpFaultType::kNetworkError:
                // Point to non-existent endpoint
                request.url("http://chaos.invalid/");
                break;
                
            case HttpFaultType::kSlowResponse:
                // Add delay before request
                engine::SleepFor(std::chrono::milliseconds(
                    static_cast<int>(fault.probability * 10000))); // Up to 10 seconds
                break;
        }
    }
    
    void RemoveFault(const HttpFault& fault) {
        active_faults_.erase(
            std::remove_if(active_faults_.begin(), active_faults_.end(),
                          [&fault](const HttpFault& f) {
                              return f.type == fault.type &&
                                     f.target_url_pattern == fault.target_url_pattern;
                          }),
            active_faults_.end());
        
        if (active_faults_.empty()) {
            http_client_.RemoveRequestInterceptor();
            interceptor_installed_ = false;
        }
    }
    
    clients::http::Client& http_client_;
    std::vector<HttpFault> active_faults_;
    bool interceptor_installed_{false};
};
```

## Chaos Testing Framework

### Experiment Orchestration
```cpp
class ChaosTestSuite {
public:
    struct TestScenario {
        std::string name;
        std::string description;
        std::vector<std::unique_ptr<ChaosExperiment>> experiments;
        std::function<bool()> preconditions;
        std::function<void()> cleanup;
    };
    
    void AddScenario(std::unique_ptr<TestScenario> scenario) {
        scenarios_.push_back(std::move(scenario));
    }
    
    struct TestResults {
        size_t total_scenarios{0};
        size_t passed_scenarios{0};
        size_t failed_scenarios{0};
        std::vector<std::string> failure_details;
        std::chrono::milliseconds total_duration{0};
    };
    
    TestResults RunAllScenarios() {
        TestResults results;
        auto start_time = std::chrono::steady_clock::now();
        
        for (auto& scenario : scenarios_) {
            results.total_scenarios++;
            
            LOG_INFO() << "Running chaos scenario: " << scenario->name;
            
            try {
                // Check preconditions
                if (!scenario->preconditions()) {
                    results.failed_scenarios++;
                    results.failure_details.push_back(
                        scenario->name + ": Preconditions not met");
                    continue;
                }
                
                // Run experiments
                bool scenario_passed = true;
                for (auto& experiment : scenario->experiments) {
                    auto result = experiment->RunExperiment();
                    if (!result.hypothesis_validated) {
                        scenario_passed = false;
                        results.failure_details.push_back(
                            scenario->name + ": Experiment failed");
                        break;
                    }
                }
                
                if (scenario_passed) {
                    results.passed_scenarios++;
                } else {
                    results.failed_scenarios++;
                }
                
                // Cleanup
                scenario->cleanup();
                
            } catch (const std::exception& ex) {
                results.failed_scenarios++;
                results.failure_details.push_back(
                    scenario->name + ": Exception - " + ex.what());
            }
        }
        
        results.total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time);
        
        return results;
    }
    
private:
    std::vector<std::unique_ptr<TestScenario>> scenarios_;
};
```

### Monitoring and Observability
```cpp
class ChaosMonitor {
public:
    struct SystemMetrics {
        double cpu_usage;
        double memory_usage;
        size_t active_connections;
        std::chrono::milliseconds avg_response_time;
        double error_rate;
        size_t requests_per_second;
    };
    
    ChaosMonitor(utils::statistics::MetricsStorage& metrics_storage)
        : metrics_storage_(metrics_storage) {}
    
    void StartMonitoring() {
        monitoring_active_ = true;
        
        monitoring_task_ = engine::AsyncNoSpan([this]() {
            while (monitoring_active_) {
                CollectMetrics();
                engine::SleepFor(std::chrono::seconds(1));
            }
        });
    }
    
    void StopMonitoring() {
        monitoring_active_ = false;
        if (monitoring_task_.IsValid()) {
            monitoring_task_.Get();
        }
    }
    
    SystemMetrics GetCurrentMetrics() const {
        std::shared_lock lock(metrics_mutex_);
        return current_metrics_;
    }
    
    std::vector<SystemMetrics> GetMetricsHistory() const {
        std::shared_lock lock(metrics_mutex_);
        return metrics_history_;
    }
    
    bool DetectAnomaly(const SystemMetrics& baseline,
                      const SystemMetrics& current,
                      double threshold = 0.2) const {
        // Simple anomaly detection based on percentage change
        auto cpu_change = std::abs(current.cpu_usage - baseline.cpu_usage) / baseline.cpu_usage;
        auto memory_change = std::abs(current.memory_usage - baseline.memory_usage) / baseline.memory_usage;
        auto response_time_change = std::abs(
            current.avg_response_time.count() - baseline.avg_response_time.count()) /
            static_cast<double>(baseline.avg_response_time.count());
        
        return cpu_change > threshold ||
               memory_change > threshold ||
               response_time_change > threshold ||
               current.error_rate > baseline.error_rate + threshold;
    }
    
private:
    void CollectMetrics() {
        SystemMetrics metrics;
        
        // Collect system metrics (simplified)
        metrics.cpu_usage = GetCpuUsage();
        metrics.memory_usage = GetMemoryUsage();
        metrics.active_connections = GetActiveConnections();
        metrics.avg_response_time = GetAverageResponseTime();
        metrics.error_rate = GetErrorRate();
        metrics.requests_per_second = GetRequestsPerSecond();
        
        {
            std::unique_lock lock(metrics_mutex_);
            current_metrics_ = metrics;
            metrics_history_.push_back(metrics);
            
            // Keep only last 1000 entries
            if (metrics_history_.size() > 1000) {
                metrics_history_.erase(metrics_history_.begin());
            }
        }
    }
    
    double GetCpuUsage() {
        // Implementation would read from /proc/stat or similar
        return 0.0;
    }
    
    double GetMemoryUsage() {
        // Implementation would read from /proc/meminfo or similar
        return 0.0;
    }
    
    size_t GetActiveConnections() {
        // Implementation would query connection pools
        return 0;
    }
    
    std::chrono::milliseconds GetAverageResponseTime() {
        // Implementation would query metrics storage
        return std::chrono::milliseconds(0);
    }
    
    double GetErrorRate() {
        // Implementation would calculate from metrics
        return 0.0;
    }
    
    size_t GetRequestsPerSecond() {
        // Implementation would calculate from metrics
        return 0;
    }
    
    utils::statistics::MetricsStorage& metrics_storage_;
    
    std::atomic<bool> monitoring_active_{false};
    engine::TaskWithResult<void> monitoring_task_;
    
    mutable std::shared_mutex metrics_mutex_;
    SystemMetrics current_metrics_;
    std::vector<SystemMetrics> metrics_history_;
};
```

## Best Practices

### Chaos Testing Guidelines

1. **Start Small**: Begin with low-impact experiments
2. **Hypothesis-Driven**: Always have a clear hypothesis
3. **Gradual Increase**: Gradually increase chaos intensity
4. **Monitor Continuously**: Implement comprehensive monitoring
5. **Automate Recovery**: Ensure automatic fault recovery
6. **Document Results**: Keep detailed records of experiments

### Safety Measures

```cpp
class ChaosSafetyController {
public:
    struct SafetyLimits {
        double max_error_rate{0.1}; // 10%
        std::chrono::milliseconds max_response_time{std::chrono::seconds(30)};
        double max_cpu_usage{0.9}; // 90%
        double max_memory_usage{0.9}; // 90%
    };
    
    ChaosSafetyController(const SafetyLimits& limits,
                         ChaosMonitor& monitor)
        : limits_(limits), monitor_(monitor) {}
    
    bool IsSafeToRunExperiment() {
        auto metrics = monitor_.GetCurrentMetrics();
        
        return metrics.error_rate < limits_.max_error_rate &&
               metrics.avg_response_time < limits_.max_response_time &&
               metrics.cpu_usage < limits_.max_cpu_usage &&
               metrics.memory_usage < limits_.max_memory_usage;
    }
    
    void EmergencyStop() {
        LOG_CRITICAL() << "Emergency stop triggered - stopping all chaos experiments";
        // Implementation would stop all active chaos injectors
    }
    
private:
    SafetyLimits limits_;
    ChaosMonitor& monitor_;
};
```

These chaos testing patterns provide a comprehensive framework for implementing resilience testing in userver-based applications. They help identify weaknesses and build confidence in system reliability under adverse conditions.