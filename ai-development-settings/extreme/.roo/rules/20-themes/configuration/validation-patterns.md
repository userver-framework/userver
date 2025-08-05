# Configuration Validation Patterns

## Overview

Configuration validation in userver ensures correctness, consistency, and security of application configuration across static and dynamic configuration systems. This comprehensive validation framework prevents runtime errors and maintains system reliability.

## Core Principles

### Validation Strategy
- **Early Validation**: Catch configuration errors at startup or update time
- **Schema-Based Validation**: Use formal schemas for configuration structure
- **Type Safety**: Enforce strong typing for configuration values
- **Business Logic Validation**: Validate configuration constraints and relationships

### Validation Layers
1. **Syntax Validation**: YAML/JSON syntax correctness
2. **Schema Validation**: Structure and type validation
3. **Semantic Validation**: Business logic and constraint validation
4. **Integration Validation**: Cross-component configuration consistency

## Static Configuration Validation

### Component Schema Definition
```cpp
// Define comprehensive configuration schema
yaml_config::Schema Component::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<components::ComponentBase>(R"(
type: object
description: HTTP client component configuration
additionalProperties: false
properties:
  base-url:
    type: string
    format: uri
    description: Base URL for HTTP requests
  timeout:
    type: string
    pattern: "^[0-9]+[smh]$"
    description: Request timeout (e.g., "30s", "5m")
    default: "30s"
  retries:
    type: integer
    minimum: 0
    maximum: 10
    description: Number of retry attempts
    default: 3
  pool-size:
    type: integer
    minimum: 1
    maximum: 1000
    description: Connection pool size
    default: 10
  headers:
    type: object
    additionalProperties:
      type: string
    description: Default HTTP headers
required:
  - base-url
)");
}
```

### Advanced Schema Patterns
```cpp
// Complex validation with conditional requirements
yaml_config::Schema GetDatabaseSchema() {
    return yaml_config::Schema::FromString(R"(
type: object
description: Database configuration
properties:
  type:
    type: string
    enum: ["postgresql", "mysql", "mongodb"]
    description: Database type
  connection:
    type: object
    properties:
      host:
        type: string
        description: Database host
      port:
        type: integer
        minimum: 1
        maximum: 65535
        description: Database port
      database:
        type: string
        minLength: 1
        description: Database name
      ssl:
        type: object
        properties:
          enabled:
            type: boolean
            default: false
          cert_file:
            type: string
            description: SSL certificate file path
          key_file:
            type: string
            description: SSL private key file path
        if:
          properties:
            enabled:
              const: true
        then:
          required: ["cert_file", "key_file"]
required: ["type", "connection"]
)");
}
```

### Configuration Validation Enforcement
```cpp
// Force validation for critical components
template<>
inline constexpr bool components::kHasValidate<CriticalComponent> = true;

// Component-specific validation mode
template<>
inline constexpr auto components::kConfigFileMode<CriticalComponent> = 
    components::ConfigFileMode::kRequired;
```

### Custom Validation Logic
```cpp
class DatabaseComponent : public components::ComponentBase {
public:
    DatabaseComponent(const components::ComponentConfig& config,
                     const components::ComponentContext& context)
        : components::ComponentBase(config, context) {
        
        // Validate configuration during construction
        ValidateConfiguration(config);
        
        // Initialize with validated configuration
        InitializeDatabase(config);
    }

private:
    void ValidateConfiguration(const components::ComponentConfig& config) {
        // Validate connection string format
        const auto connection_string = config["connection-string"].As<std::string>();
        if (!IsValidConnectionString(connection_string)) {
            throw std::runtime_error("Invalid database connection string format");
        }
        
        // Validate pool size constraints
        const auto pool_size = config["pool-size"].As<int>();
        const auto max_connections = config["max-connections"].As<int>();
        if (pool_size > max_connections) {
            throw std::runtime_error("Pool size cannot exceed max connections");
        }
        
        // Validate SSL configuration consistency
        const auto ssl_enabled = config["ssl"]["enabled"].As<bool>(false);
        if (ssl_enabled) {
            config["ssl"]["cert-file"].CheckNotMissing();
            config["ssl"]["key-file"].CheckNotMissing();
        }
    }
    
    static bool IsValidConnectionString(const std::string& connection_string) {
        // Implement connection string validation logic
        return connection_string.find("://") != std::string::npos;
    }
};
```

## Dynamic Configuration Validation

### Type-Safe Dynamic Configuration
```cpp
// Define validated dynamic configuration types
struct HttpClientConfig {
    std::chrono::milliseconds timeout;
    int max_retries;
    std::size_t pool_size;
    
    // Validation during parsing
    void Validate() const {
        if (timeout.count() <= 0) {
            throw std::runtime_error("Timeout must be positive");
        }
        if (max_retries < 0 || max_retries > 10) {
            throw std::runtime_error("Retries must be between 0 and 10");
        }
        if (pool_size == 0 || pool_size > 1000) {
            throw std::runtime_error("Pool size must be between 1 and 1000");
        }
    }
};

// Parser with validation
HttpClientConfig Parse(const formats::json::Value& value,
                      formats::parse::To<HttpClientConfig>) {
    auto config = HttpClientConfig{
        value["timeout_ms"].As<std::chrono::milliseconds>(),
        value["max_retries"].As<int>(),
        value["pool_size"].As<std::size_t>()
    };
    
    // Validate parsed configuration
    config.Validate();
    return config;
}

// Dynamic configuration key with validation
inline const dynamic_config::Key<HttpClientConfig> kHttpClientConfig{
    "HTTP_CLIENT_CONFIG",
    dynamic_config::DefaultAsJsonString{R"({
        "timeout_ms": 30000,
        "max_retries": 3,
        "pool_size": 10
    })"}
};
```

### Enum Validation Patterns
```cpp
// Validated enum configuration
enum class LogLevel { kDebug, kInfo, kWarning, kError };

LogLevel Parse(const formats::json::Value& value, 
               formats::parse::To<LogLevel>) {
    static constexpr utils::TrivialBiMap kMap([](auto selector) {
        return selector()
            .Case(LogLevel::kDebug, "debug")
            .Case(LogLevel::kInfo, "info")
            .Case(LogLevel::kWarning, "warning")
            .Case(LogLevel::kError, "error");
    });
    
    try {
        return utils::ParseFromValueString(value, kMap);
    } catch (const std::exception& e) {
        throw std::runtime_error(
            fmt::format("Invalid log level '{}'. Valid values: debug, info, warning, error", 
                       value.As<std::string>()));
    }
}
```

### Complex Object Validation
```cpp
// Multi-field validation with cross-field constraints
struct DatabasePoolConfig {
    std::size_t min_connections;
    std::size_t max_connections;
    std::chrono::seconds idle_timeout;
    std::chrono::seconds connection_timeout;
    
    void Validate() const {
        if (min_connections == 0) {
            throw std::runtime_error("Minimum connections must be at least 1");
        }
        if (max_connections < min_connections) {
            throw std::runtime_error("Maximum connections must be >= minimum connections");
        }
        if (max_connections > 1000) {
            throw std::runtime_error("Maximum connections cannot exceed 1000");
        }
        if (idle_timeout.count() <= 0) {
            throw std::runtime_error("Idle timeout must be positive");
        }
        if (connection_timeout.count() <= 0) {
            throw std::runtime_error("Connection timeout must be positive");
        }
        if (connection_timeout > std::chrono::minutes{5}) {
            throw std::runtime_error("Connection timeout cannot exceed 5 minutes");
        }
    }
};
```

## Validation Testing Patterns

### Schema Validation Testing
```cpp
class ConfigValidationTest : public ::testing::Test {
protected:
    void ValidateConfigSchema(const std::string& config_yaml) {
        const auto config_value = formats::yaml::FromString(config_yaml);
        const auto schema = Component::GetStaticConfigSchema();
        
        // This should not throw
        EXPECT_NO_THROW(schema.Validate(config_value));
    }
    
    void ExpectConfigValidationError(const std::string& config_yaml,
                                   const std::string& expected_error) {
        const auto config_value = formats::yaml::FromString(config_yaml);
        const auto schema = Component::GetStaticConfigSchema();
        
        EXPECT_THROW({
            try {
                schema.Validate(config_value);
            } catch (const std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(expected_error));
                throw;
            }
        }, std::exception);
    }
};

TEST_F(ConfigValidationTest, ValidConfiguration) {
    ValidateConfigSchema(R"(
base-url: "https://api.example.com"
timeout: "30s"
retries: 3
pool-size: 10
)");
}

TEST_F(ConfigValidationTest, InvalidTimeout) {
    ExpectConfigValidationError(R"(
base-url: "https://api.example.com"
timeout: "invalid"
)", "timeout");
}

TEST_F(ConfigValidationTest, MissingRequiredField) {
    ExpectConfigValidationError(R"(
timeout: "30s"
retries: 3
)", "base-url");
}
```

### Dynamic Configuration Validation Testing
```cpp
TEST(DynamicConfigTest, ValidConfiguration) {
    const auto config_json = formats::json::FromString(R"({
        "timeout_ms": 5000,
        "max_retries": 2,
        "pool_size": 20
    })");
    
    EXPECT_NO_THROW({
        auto config = config_json.As<HttpClientConfig>();
        // Configuration should be valid
    });
}

TEST(DynamicConfigTest, InvalidConfiguration) {
    const auto config_json = formats::json::FromString(R"({
        "timeout_ms": -1000,
        "max_retries": 15,
        "pool_size": 0
    })");
    
    EXPECT_THROW({
        auto config = config_json.As<HttpClientConfig>();
    }, std::runtime_error);
}
```

### Testsuite Configuration Validation
```python
# Test configuration validation in testsuite
async def test_invalid_dynamic_config(service_client, dynamic_config):
    # Test that invalid configuration is rejected
    with pytest.raises(Exception):
        dynamic_config.set_values({
            'HTTP_CLIENT_CONFIG': {
                'timeout_ms': -1000,  # Invalid: negative timeout
                'max_retries': 15,    # Invalid: too many retries
                'pool_size': 0        # Invalid: zero pool size
            }
        })
        await service_client.update_server_state()

async def test_configuration_constraints(service_client, dynamic_config):
    # Test cross-field validation
    with pytest.raises(Exception):
        dynamic_config.set_values({
            'DATABASE_POOL_CONFIG': {
                'min_connections': 10,
                'max_connections': 5,  # Invalid: max < min
                'idle_timeout': '30s',
                'connection_timeout': '10s'
            }
        })
        await service_client.update_server_state()
```

## Validation Error Handling

### Structured Error Reporting
```cpp
class ConfigurationError : public std::runtime_error {
public:
    ConfigurationError(const std::string& component,
                      const std::string& field,
                      const std::string& message)
        : std::runtime_error(FormatError(component, field, message)),
          component_(component),
          field_(field),
          message_(message) {}
    
    const std::string& GetComponent() const { return component_; }
    const std::string& GetField() const { return field_; }
    const std::string& GetMessage() const { return message_; }

private:
    static std::string FormatError(const std::string& component,
                                 const std::string& field,
                                 const std::string& message) {
        return fmt::format("Configuration error in component '{}', field '{}': {}", 
                          component, field, message);
    }
    
    std::string component_;
    std::string field_;
    std::string message_;
};
```

### Validation Result Aggregation
```cpp
class ValidationResult {
public:
    void AddError(const std::string& field, const std::string& message) {
        errors_.emplace_back(field, message);
    }
    
    void AddWarning(const std::string& field, const std::string& message) {
        warnings_.emplace_back(field, message);
    }
    
    bool IsValid() const { return errors_.empty(); }
    
    void ThrowIfInvalid(const std::string& component) const {
        if (!IsValid()) {
            std::string error_message = fmt::format(
                "Configuration validation failed for component '{}':\n", component);
            
            for (const auto& [field, message] : errors_) {
                error_message += fmt::format("  - {}: {}\n", field, message);
            }
            
            throw ConfigurationError(component, "", error_message);
        }
    }

private:
    std::vector<std::pair<std::string, std::string>> errors_;
    std::vector<std::pair<std::string, std::string>> warnings_;
};
```

## Advanced Validation Patterns

### Conditional Validation
```cpp
// Validation based on configuration context
class ConditionalValidator {
public:
    static void ValidateHttpsConfig(const yaml_config::YamlConfig& config) {
        const auto use_https = config["use-https"].As<bool>(false);
        
        if (use_https) {
            // HTTPS-specific validation
            config["ssl-cert-path"].CheckNotMissing();
            config["ssl-key-path"].CheckNotMissing();
            
            const auto cert_path = config["ssl-cert-path"].As<std::string>();
            const auto key_path = config["ssl-key-path"].As<std::string>();
            
            if (!std::filesystem::exists(cert_path)) {
                throw ConfigurationError("https", "ssl-cert-path", 
                                       "Certificate file does not exist");
            }
            
            if (!std::filesystem::exists(key_path)) {
                throw ConfigurationError("https", "ssl-key-path", 
                                       "Private key file does not exist");
            }
        }
    }
};
```

### Cross-Component Validation
```cpp
class SystemValidator {
public:
    static void ValidateSystemConfiguration(
        const components::ComponentContext& context) {
        
        // Validate database and cache consistency
        ValidateDatabaseCacheConsistency(context);
        
        // Validate service discovery configuration
        ValidateServiceDiscovery(context);
        
        // Validate resource limits
        ValidateResourceLimits(context);
    }

private:
    static void ValidateDatabaseCacheConsistency(
        const components::ComponentContext& context) {
        
        const auto& db_component = context.FindComponent<DatabaseComponent>();
        const auto& cache_component = context.FindComponent<CacheComponent>();
        
        const auto db_timeout = db_component.GetTimeout();
        const auto cache_timeout = cache_component.GetTimeout();
        
        if (cache_timeout >= db_timeout) {
            throw ConfigurationError("system", "timeouts",
                "Cache timeout must be less than database timeout");
        }
    }
};
```

### Runtime Validation
```cpp
class RuntimeValidator {
public:
    static void ValidateRuntimeState(const Component& component) {
        // Validate current configuration against runtime constraints
        const auto current_load = component.GetCurrentLoad();
        const auto max_load = component.GetMaxLoad();
        
        if (current_load > max_load * 0.9) {
            LOG_WARNING() << "Component approaching maximum load capacity";
        }
        
        // Validate resource availability
        const auto available_memory = GetAvailableMemory();
        const auto required_memory = component.GetRequiredMemory();
        
        if (available_memory < required_memory * 1.2) {
            throw std::runtime_error("Insufficient memory for current configuration");
        }
    }
};
```

## Best Practices

### Validation Design Principles
- **Fail Fast**: Validate configuration as early as possible
- **Clear Messages**: Provide specific, actionable error messages
- **Comprehensive Coverage**: Validate all configuration aspects
- **Performance Aware**: Keep validation efficient for hot paths

### Schema Design Guidelines
```yaml
# Good schema design
properties:
  timeout:
    type: string
    pattern: "^[0-9]+[smh]$"
    description: "Timeout duration (e.g., '30s', '5m', '1h')"
    examples: ["30s", "5m", "1h"]
  
  pool_size:
    type: integer
    minimum: 1
    maximum: 1000
    description: "Connection pool size"
    default: 10
    
  log_level:
    type: string
    enum: ["debug", "info", "warning", "error"]
    description: "Logging level"
    default: "info"
```

### Error Message Guidelines
```cpp
// Good error messages
throw ConfigurationError("database", "pool_size", 
    "Pool size must be between 1 and 1000, got: " + std::to_string(pool_size));

// Include context and suggestions
throw ConfigurationError("http-client", "base_url",
    "Invalid URL format. Expected format: 'https://example.com/api'");

// Provide examples for complex formats
throw ConfigurationError("scheduler", "cron_expression",
    "Invalid cron expression. Examples: '0 */5 * * *' (every 5 minutes), "
    "'0 0 * * 0' (weekly)");
```

### Validation Performance
```cpp
// Cache expensive validations
class ValidationCache {
public:
    static bool IsValidUrl(const std::string& url) {
        static std::unordered_set<std::string> valid_urls;
        static std::unordered_set<std::string> invalid_urls;
        
        if (valid_urls.count(url)) return true;
        if (invalid_urls.count(url)) return false;
        
        bool is_valid = PerformUrlValidation(url);
        (is_valid ? valid_urls : invalid_urls).insert(url);
        return is_valid;
    }
};
```

## Monitoring and Observability

### Validation Metrics
```cpp
class ValidationMetrics {
public:
    static void RecordValidationResult(const std::string& component,
                                     const std::string& type,
                                     bool success) {
        auto& stats = utils::statistics::GetCurrentStatistics();
        stats.GetMetric("config.validation")
            .WithTag("component", component)
            .WithTag("type", type)
            .WithTag("success", success ? "true" : "false")
            .Increment();
    }
    
    static void RecordValidationLatency(const std::string& component,
                                      std::chrono::milliseconds duration) {
        auto& stats = utils::statistics::GetCurrentStatistics();
        stats.GetMetric("config.validation.duration_ms")
            .WithTag("component", component)
            .Record(duration.count());
    }
};
```

### Validation Logging
```cpp
// Structured validation logging
LOG_INFO() << "Configuration validation completed"
           << logging::LogExtra{
               {"component", component_name},
               {"validation_time_ms", validation_duration.count()},
               {"errors_count", validation_result.GetErrorCount()},
               {"warnings_count", validation_result.GetWarningCount()}
           };
```

## Cross-References

- **Static Configuration**: [`static-config.md`](./static-config.md) - Static configuration patterns
- **Dynamic Configuration**: [`dynamic-config.md`](./dynamic-config.md) - Runtime configuration updates
- **Environment Handling**: [`environment-handling.md`](./environment-handling.md) - Multi-environment configuration
- **Component System**: [`../../memory-bank/main/component-system.md`](../../memory-bank/main/component-system.md) - Component architecture
- **Service Patterns**: [`../../memory-bank/main/service-patterns.md`](../../memory-bank/main/service-patterns.md) - Service design patterns