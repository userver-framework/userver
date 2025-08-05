# Error Investigation

## Overview

Systematic error analysis and root cause investigation patterns for userver applications, covering exception handling, error propagation, crash analysis, and diagnostic techniques.

## Error Classification Framework

### Error Categories

**System Errors**
```cpp
enum class SystemErrorType {
    kMemoryExhaustion,    // Out of memory, allocation failures
    kResourceLimits,      // File descriptors, thread limits
    kNetworkFailures,     // Connection timeouts, DNS failures
    kFileSystemErrors,    // Disk full, permission denied
    kSignalHandling       // SIGTERM, SIGKILL, SIGSEGV
};
```

**Application Errors**
```cpp
enum class ApplicationErrorType {
    kLogicErrors,         // Business logic failures
    kValidationErrors,    // Input validation failures
    kConfigurationErrors, // Invalid configuration
    kDependencyErrors,    // External service failures
    kDataCorruption      // Inconsistent data state
};
```

**Framework Errors**
```cpp
enum class FrameworkErrorType {
    kTaskCancellation,    // Coroutine cancellation
    kDeadlineExceeded,    // Request timeout
    kComponentFailure,    // Component initialization failure
    kSerializationError,  // JSON/BSON parsing errors
    kDatabaseError       // Connection pool exhaustion
};
```

## Error Detection Strategies

### Structured Error Handling

**Error Context Preservation**
```cpp
// Comprehensive error context
class ErrorContext {
private:
    std::string operation_name_;
    std::unordered_map<std::string, std::string> context_data_;
    std::vector<std::string> call_stack_;
    std::chrono::steady_clock::time_point timestamp_;
    
public:
    ErrorContext(std::string operation) 
        : operation_name_(std::move(operation))
        , timestamp_(std::chrono::steady_clock::now()) {}
    
    ErrorContext& AddContext(const std::string& key, const std::string& value) {
        context_data_[key] = value;
        return *this;
    }
    
    ErrorContext& AddStackFrame(const std::string& frame) {
        call_stack_.push_back(frame);
        return *this;
    }
    
    void LogError(const std::exception& e) const {
        logging::LogExtra log_extra;
        log_extra.Extend("operation", operation_name_);
        log_extra.Extend("error_message", e.what());
        log_extra.Extend("timestamp", std::to_string(timestamp_.time_since_epoch().count()));
        
        for (const auto& [key, value] : context_data_) {
            log_extra.Extend(key, value);
        }
        
        LOG_ERROR() << log_extra << "Error occurred in operation: " << operation_name_
                    << logging::LogExtra::Stacktrace();
    }
};

// Usage pattern
void ProcessRequest(const Request& request) {
    ErrorContext ctx("process_request");
    ctx.AddContext("request_id", request.id())
       .AddContext("user_id", request.user_id())
       .AddContext("endpoint", request.endpoint());
    
    try {
        // Process request
        auto result = ProcessBusinessLogic(request);
        SendResponse(result);
    } catch (const std::exception& e) {
        ctx.LogError(e);
        throw;
    }
}
```

### Exception Hierarchy

**Custom Exception Types**
```cpp
// Base exception with rich context
class UServerException : public std::exception {
private:
    std::string message_;
    std::string operation_;
    std::unordered_map<std::string, std::string> context_;
    mutable std::string formatted_message_;
    
public:
    UServerException(std::string message, std::string operation)
        : message_(std::move(message))
        , operation_(std::move(operation)) {}
    
    UServerException& AddContext(const std::string& key, const std::string& value) {
        context_[key] = value;
        formatted_message_.clear(); // Reset cached message
        return *this;
    }
    
    const char* what() const noexcept override {
        if (formatted_message_.empty()) {
            formatted_message_ = FormatMessage();
        }
        return formatted_message_.c_str();
    }
    
    const std::string& GetOperation() const { return operation_; }
    const auto& GetContext() const { return context_; }
    
private:
    std::string FormatMessage() const {
        std::ostringstream oss;
        oss << "Operation: " << operation_ << ", Error: " << message_;
        
        if (!context_.empty()) {
            oss << ", Context: {";
            bool first = true;
            for (const auto& [key, value] : context_) {
                if (!first) oss << ", ";
                oss << key << "=" << value;
                first = false;
            }
            oss << "}";
        }
        
        return oss.str();
    }
};

// Specific exception types
class DatabaseException : public UServerException {
public:
    DatabaseException(const std::string& message, const std::string& query)
        : UServerException(message, "database_operation") {
        AddContext("query", query);
    }
};

class NetworkException : public UServerException {
public:
    NetworkException(const std::string& message, const std::string& endpoint)
        : UServerException(message, "network_operation") {
        AddContext("endpoint", endpoint);
    }
};

class ValidationException : public UServerException {
public:
    ValidationException(const std::string& message, const std::string& field)
        : UServerException(message, "validation") {
        AddContext("field", field);
    }
};
```

## Error Analysis Techniques

### Log Analysis Patterns

**Error Pattern Detection**
```cpp
// Automated error pattern analysis
class ErrorPatternAnalyzer {
private:
    struct ErrorPattern {
        std::string pattern_name;
        std::regex error_regex;
        size_t occurrence_count = 0;
        std::chrono::steady_clock::time_point first_seen;
        std::chrono::steady_clock::time_point last_seen;
    };
    
    std::vector<ErrorPattern> patterns_;
    
public:
    ErrorPatternAnalyzer() {
        // Define common error patterns
        patterns_.emplace_back(ErrorPattern{
            "database_timeout",
            std::regex(R"(database.*timeout|connection.*timeout)"),
            0, {}, {}
        });
        
        patterns_.emplace_back(ErrorPattern{
            "memory_exhaustion", 
            std::regex(R"(out of memory|allocation failed|bad_alloc)"),
            0, {}, {}
        });
        
        patterns_.emplace_back(ErrorPattern{
            "network_failure",
            std::regex(R"(connection refused|network unreachable|dns resolution)"),
            0, {}, {}
        });
    }
    
    void AnalyzeLogEntry(const std::string& log_entry) {
        auto now = std::chrono::steady_clock::now();
        
        for (auto& pattern : patterns_) {
            if (std::regex_search(log_entry, pattern.error_regex)) {
                pattern.occurrence_count++;
                pattern.last_seen = now;
                
                if (pattern.occurrence_count == 1) {
                    pattern.first_seen = now;
                }
                
                // Alert on pattern frequency
                if (pattern.occurrence_count % 100 == 0) {
                    LOG_WARNING() << "Error pattern detected: " << pattern.pattern_name
                                 << " occurrences=" << pattern.occurrence_count
                                 << " time_span=" << (now - pattern.first_seen).count() << "ns";
                }
            }
        }
    }
    
    void GenerateErrorReport() {
        LOG_INFO() << "Error Pattern Analysis Report:";
        for (const auto& pattern : patterns_) {
            if (pattern.occurrence_count > 0) {
                auto time_span = pattern.last_seen - pattern.first_seen;
                LOG_INFO() << "Pattern: " << pattern.pattern_name
                          << " count=" << pattern.occurrence_count
                          << " time_span=" << time_span.count() << "ns"
                          << " rate=" << (pattern.occurrence_count * 1e9 / time_span.count()) << "/sec";
            }
        }
    }
};
```

### Stack Trace Analysis

**Advanced Stack Trace Processing**
```cpp
// Stack trace analysis utilities
class StackTraceAnalyzer {
public:
    struct StackFrame {
        std::string function_name;
        std::string file_name;
        int line_number;
        std::string module_name;
    };
    
    std::vector<StackFrame> ParseStackTrace(const std::string& stack_trace) {
        std::vector<StackFrame> frames;
        std::istringstream iss(stack_trace);
        std::string line;
        
        std::regex frame_regex(R"(#(\d+)\s+(.+)\s+at\s+(.+):(\d+))");
        std::smatch match;
        
        while (std::getline(iss, line)) {
            if (std::regex_search(line, match, frame_regex)) {
                StackFrame frame;
                frame.function_name = match[2].str();
                frame.file_name = match[3].str();
                frame.line_number = std::stoi(match[4].str());
                frames.push_back(std::move(frame));
            }
        }
        
        return frames;
    }
    
    void AnalyzeCrashPattern(const std::vector<StackFrame>& frames) {
        if (frames.empty()) return;
        
        // Analyze crash location
        const auto& crash_frame = frames[0];
        LOG_ERROR() << "Crash analysis:"
                   << " function=" << crash_frame.function_name
                   << " file=" << crash_frame.file_name
                   << " line=" << crash_frame.line_number;
        
        // Look for common crash patterns
        for (const auto& frame : frames) {
            if (frame.function_name.find("mutex") != std::string::npos ||
                frame.function_name.find("lock") != std::string::npos) {
                LOG_WARNING() << "Potential deadlock detected in stack trace";
                break;
            }
            
            if (frame.function_name.find("alloc") != std::string::npos ||
                frame.function_name.find("malloc") != std::string::npos) {
                LOG_WARNING() << "Memory allocation issue detected in stack trace";
                break;
            }
        }
    }
    
    std::string GenerateStackTraceFingerprint(const std::vector<StackFrame>& frames) {
        std::ostringstream oss;
        
        // Use top 5 frames for fingerprint
        size_t frame_count = std::min(frames.size(), size_t{5});
        for (size_t i = 0; i < frame_count; ++i) {
            if (i > 0) oss << "|";
            oss << frames[i].function_name;
        }
        
        return oss.str();
    }
};
```

## Root Cause Investigation

### Correlation Analysis

**Multi-Dimensional Error Correlation**
```cpp
// Error correlation engine
class ErrorCorrelationEngine {
private:
    struct ErrorEvent {
        std::string error_type;
        std::string component;
        std::chrono::steady_clock::time_point timestamp;
        std::unordered_map<std::string, std::string> attributes;
    };
    
    std::vector<ErrorEvent> error_history_;
    
public:
    void RecordError(const std::string& error_type,
                    const std::string& component,
                    const std::unordered_map<std::string, std::string>& attributes) {
        error_history_.emplace_back(ErrorEvent{
            error_type, component, std::chrono::steady_clock::now(), attributes
        });
        
        // Analyze correlations periodically
        if (error_history_.size() % 100 == 0) {
            AnalyzeCorrelations();
        }
    }
    
    void AnalyzeCorrelations() {
        auto now = std::chrono::steady_clock::now();
        auto time_window = std::chrono::minutes(5);
        
        // Find errors within time window
        std::vector<ErrorEvent> recent_errors;
        for (const auto& error : error_history_) {
            if (now - error.timestamp <= time_window) {
                recent_errors.push_back(error);
            }
        }
        
        // Analyze patterns
        std::unordered_map<std::string, size_t> error_counts;
        std::unordered_map<std::string, size_t> component_counts;
        
        for (const auto& error : recent_errors) {
            error_counts[error.error_type]++;
            component_counts[error.component]++;
        }
        
        // Detect anomalies
        for (const auto& [error_type, count] : error_counts) {
            if (count > 10) { // Threshold
                LOG_WARNING() << "Error spike detected: " << error_type
                             << " count=" << count
                             << " time_window=" << time_window.count() << "min";
            }
        }
        
        // Component correlation
        for (const auto& [component, count] : component_counts) {
            if (count > 5) {
                LOG_WARNING() << "Component error concentration: " << component
                             << " error_count=" << count;
            }
        }
    }
    
    std::vector<std::string> FindCorrelatedErrors(const std::string& primary_error) {
        std::vector<std::string> correlated;
        auto time_window = std::chrono::minutes(1);
        
        for (const auto& primary : error_history_) {
            if (primary.error_type != primary_error) continue;
            
            // Find errors within time window
            for (const auto& candidate : error_history_) {
                if (candidate.error_type == primary_error) continue;
                
                auto time_diff = std::abs((candidate.timestamp - primary.timestamp).count());
                if (time_diff <= time_window.count()) {
                    correlated.push_back(candidate.error_type);
                }
            }
        }
        
        return correlated;
    }
};
```

### Diagnostic Data Collection

**Comprehensive Diagnostics**
```cpp
// Diagnostic data collector
class DiagnosticCollector {
public:
    struct SystemDiagnostics {
        double cpu_usage;
        size_t memory_usage;
        size_t disk_usage;
        size_t network_connections;
        size_t thread_count;
        std::chrono::steady_clock::time_point timestamp;
    };
    
    struct ApplicationDiagnostics {
        size_t active_requests;
        size_t queue_depth;
        size_t cache_hit_rate;
        size_t database_connections;
        std::unordered_map<std::string, size_t> component_status;
    };
    
    void CollectDiagnostics() {
        auto system_diag = CollectSystemDiagnostics();
        auto app_diag = CollectApplicationDiagnostics();
        
        // Log comprehensive diagnostics
        LOG_INFO() << "System Diagnostics:"
                  << " cpu=" << system_diag.cpu_usage << "%"
                  << " memory=" << system_diag.memory_usage << "MB"
                  << " threads=" << system_diag.thread_count;
        
        LOG_INFO() << "Application Diagnostics:"
                  << " active_requests=" << app_diag.active_requests
                  << " queue_depth=" << app_diag.queue_depth
                  << " cache_hit_rate=" << app_diag.cache_hit_rate << "%"
                  << " db_connections=" << app_diag.database_connections;
        
        // Detect anomalies
        DetectSystemAnomalies(system_diag);
        DetectApplicationAnomalies(app_diag);
    }
    
private:
    void DetectSystemAnomalies(const SystemDiagnostics& diag) {
        if (diag.cpu_usage > 90.0) {
            LOG_ERROR() << "High CPU usage detected: " << diag.cpu_usage << "%";
        }
        
        if (diag.memory_usage > 8 * 1024) { // 8GB
            LOG_ERROR() << "High memory usage detected: " << diag.memory_usage << "MB";
        }
        
        if (diag.thread_count > 1000) {
            LOG_ERROR() << "High thread count detected: " << diag.thread_count;
        }
    }
    
    void DetectApplicationAnomalies(const ApplicationDiagnostics& diag) {
        if (diag.queue_depth > 1000) {
            LOG_ERROR() << "High queue depth detected: " << diag.queue_depth;
        }
        
        if (diag.cache_hit_rate < 80) {
            LOG_WARNING() << "Low cache hit rate: " << diag.cache_hit_rate << "%";
        }
        
        if (diag.database_connections > 100) {
            LOG_WARNING() << "High database connection count: " << diag.database_connections;
        }
    }
    
    SystemDiagnostics CollectSystemDiagnostics();
    ApplicationDiagnostics CollectApplicationDiagnostics();
};
```

## Crash Analysis

### Core Dump Analysis

**Automated Core Dump Processing**
```cpp
// Core dump analyzer
class CoreDumpAnalyzer {
public:
    struct CrashInfo {
        std::string signal_name;
        std::string crash_location;
        std::vector<std::string> stack_trace;
        std::unordered_map<std::string, std::string> registers;
        std::string memory_map;
    };
    
    CrashInfo AnalyzeCrashDump(const std::string& core_file_path) {
        CrashInfo info;
        
        // Extract crash information using GDB
        std::string gdb_command = "gdb -batch -ex 'bt' -ex 'info registers' "
                                 "-ex 'info proc mappings' -ex 'quit' "
                                 "binary_path " + core_file_path;
        
        auto gdb_output = ExecuteCommand(gdb_command);
        
        // Parse GDB output
        info.stack_trace = ExtractStackTrace(gdb_output);
        info.registers = ExtractRegisters(gdb_output);
        info.memory_map = ExtractMemoryMap(gdb_output);
        
        // Analyze crash pattern
        AnalyzeCrashPattern(info);
        
        return info;
    }
    
private:
    void AnalyzeCrashPattern(const CrashInfo& info) {
        // Common crash patterns
        if (info.signal_name == "SIGSEGV") {
            LOG_ERROR() << "Segmentation fault detected";
            
            // Check for null pointer dereference
            if (info.crash_location.find("0x0") != std::string::npos) {
                LOG_ERROR() << "Null pointer dereference detected";
            }
        }
        
        if (info.signal_name == "SIGABRT") {
            LOG_ERROR() << "Abort signal detected - likely assertion failure";
        }
        
        if (info.signal_name == "SIGFPE") {
            LOG_ERROR() << "Floating point exception detected";
        }
        
        // Analyze stack trace for patterns
        for (const auto& frame : info.stack_trace) {
            if (frame.find("std::terminate") != std::string::npos) {
                LOG_ERROR() << "Unhandled exception caused termination";
            }
            
            if (frame.find("__stack_chk_fail") != std::string::npos) {
                LOG_ERROR() << "Stack buffer overflow detected";
            }
        }
    }
    
    std::vector<std::string> ExtractStackTrace(const std::string& gdb_output);
    std::unordered_map<std::string, std::string> ExtractRegisters(const std::string& gdb_output);
    std::string ExtractMemoryMap(const std::string& gdb_output);
    std::string ExecuteCommand(const std::string& command);
};
```

## Error Recovery Strategies

### Graceful Degradation

**Error Recovery Patterns**
```cpp
// Resilient operation wrapper
template<typename Func, typename FallbackFunc>
class ResilientOperation {
private:
    Func primary_operation_;
    FallbackFunc fallback_operation_;
    size_t max_retries_;
    std::chrono::milliseconds retry_delay_;
    
public:
    ResilientOperation(Func primary, FallbackFunc fallback, 
                      size_t max_retries = 3,
                      std::chrono::milliseconds retry_delay = std::chrono::milliseconds(100))
        : primary_operation_(std::move(primary))
        , fallback_operation_(std::move(fallback))
        , max_retries_(max_retries)
        , retry_delay_(retry_delay) {}
    
    template<typename... Args>
    auto Execute(Args&&... args) {
        size_t attempt = 0;
        
        while (attempt < max_retries_) {
            try {
                return primary_operation_(std::forward<Args>(args)...);
            } catch (const std::exception& e) {
                attempt++;
                
                LOG_WARNING() << "Operation failed, attempt " << attempt 
                             << "/" << max_retries_ << ": " << e.what();
                
                if (attempt >= max_retries_) {
                    LOG_ERROR() << "Primary operation failed after " << max_retries_ 
                               << " attempts, falling back";
                    break;
                }
                
                // Exponential backoff
                auto delay = retry_delay_ * (1 << (attempt - 1));
                std::this_thread::sleep_for(delay);
            }
        }
        
        // Execute fallback
        try {
            return fallback_operation_(std::forward<Args>(args)...);
        } catch (const std::exception& e) {
            LOG_ERROR() << "Fallback operation also failed: " << e.what();
            throw;
        }
    }
};

// Usage example
auto resilient_db_query = ResilientOperation(
    [&](const std::string& query) {
        return database_->Execute(query);
    },
    [&](const std::string& query) {
        return cache_->GetCachedResult(query);
    }
);
```

## Integration Points

**Cross-References**
- [`troubleshooting-workflows.md`](./troubleshooting-workflows.md) - General debugging workflows
- [`performance-analysis.md`](./performance-analysis.md) - Performance-related errors
- [`profiling-techniques.md`](./profiling-techniques.md) - Error profiling methods

**Memory Bank References**
- [`troubleshooting-guide.md`](../../memory-bank/main/troubleshooting-guide.md) - General troubleshooting
- [`framework-core.md`](../../memory-bank/main/framework-core.md) - Framework error patterns
- [`chaos-testing`](../../memory-bank/specialized/chaos-testing/) - Error injection testing

## Best Practices

### Error Prevention
- Comprehensive input validation
- Defensive programming practices
- Resource management with RAII
- Proper exception handling hierarchy

### Error Detection
- Structured logging with context
- Automated error pattern detection
- Real-time monitoring and alerting
- Regular log analysis and review

### Error Response
- Graceful degradation strategies
- Circuit breaker patterns
- Retry mechanisms with backoff
- Comprehensive error reporting