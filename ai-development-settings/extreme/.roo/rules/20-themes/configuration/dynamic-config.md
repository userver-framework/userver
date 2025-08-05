# Dynamic Configuration Management

## Overview

Dynamic configuration in userver enables runtime modification of application behavior without service restarts. This system provides real-time configuration updates, fallback mechanisms, and testing support for production-ready services.

## Core Principles

### Dynamic Configuration Architecture
- **Runtime Updates**: Configuration changes applied without service restart
- **Type Safety**: Strongly-typed configuration access with compile-time validation
- **Fallback Mechanisms**: Multiple layers of fallback for configuration failures
- **Kill Switches**: Emergency configuration overrides for incident response

### Configuration Distribution
```cpp
// Dynamic configuration flow
dynamic_config::Source -> dynamic_config::Snapshot -> Application Code
```

## Implementation Patterns

### Defining Dynamic Configuration Keys
```cpp
// Define global configuration key
namespace myservice {
inline const dynamic_config::Key<int> kMyConfig{
    "SAMPLE_INTEGER_FROM_RUNTIME_CONFIG", 
    42  // default value
};

// Complex configuration with JSON default
inline const dynamic_config::Key<MyStruct> kMyStructConfig{
    "MY_STRUCT_CONFIG",
    dynamic_config::DefaultAsJsonString{R"({
        "enabled": true,
        "timeout_ms": 1000,
        "retries": 3
    })"}
};
}
```

### Accessing Dynamic Configuration
```cpp
class Component : public components::ComponentBase {
private:
    dynamic_config::Source config_;

public:
    Component(const components::ComponentConfig& config,
              const components::ComponentContext& context)
        : components::ComponentBase(config, context),
          config_(context.FindComponent<components::DynamicConfig>().GetSource()) {}

    int DoSomething() const {
        // Get current configuration snapshot
        const auto runtime_config = config_.GetSnapshot();
        return runtime_config[kMyConfig];
    }
};
```

### Configuration Parsing Patterns
```cpp
// Trivial types (parsed automatically)
inline const dynamic_config::Key<bool> kFeatureEnabled{
    "FEATURE_ENABLED", false
};

inline const dynamic_config::Key<std::string> kServiceUrl{
    "SERVICE_URL", "https://default.example.com"
};

// Duration parsing
inline const dynamic_config::Key<std::chrono::milliseconds> kTimeoutMs{
    "REQUEST_TIMEOUT_MS", std::chrono::milliseconds{1000}
};
```

### Complex Type Parsing
```cpp
// Enum configuration
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
    return utils::ParseFromValueString(value, kMap);
}

// Struct configuration
struct DatabaseConfig {
    std::string host;
    std::uint16_t port;
    std::chrono::seconds timeout;
};

DatabaseConfig Parse(const formats::json::Value& value,
                    formats::parse::To<DatabaseConfig>) {
    return DatabaseConfig{
        value["host"].As<std::string>(),
        value["port"].As<std::uint16_t>(),
        value["timeout_seconds"].As<std::chrono::seconds>()
    };
}
```

## Configuration Updates and Subscriptions

### Configuration Subscriptions
```cpp
class Component : public components::ComponentBase {
private:
    dynamic_config::Source config_;
    concurrent::AsyncEventSubscriberScope config_subscription_;

public:
    Component(const components::ComponentConfig& config,
              const components::ComponentContext& context)
        : components::ComponentBase(config, context),
          config_(context.FindComponent<components::DynamicConfig>().GetSource()),
          config_subscription_(config_.UpdateAndListen(
              this, "component-config-update", &Component::OnConfigUpdate)) {}

private:
    void OnConfigUpdate(const dynamic_config::Snapshot& config) {
        // Handle configuration updates
        const auto new_timeout = config[kTimeoutConfig];
        // Update internal state based on new configuration
    }
};
```

### Configuration Client Setup
```yaml
# Dynamic configuration client setup
dynamic-config:
  updates-enabled: true
  fs-cache-path: $config-cache
  fs-task-processor: fs-task-processor

dynamic-config-client:
  config-url: $config-server-url
  http-retries: 5
  http-timeout: 20s
  service-name: $service-name

dynamic-config-client-updater:
  config-settings: false
  first-update-fail-ok: true
  full-update-interval: 1m
  update-interval: 5s
```

## Kill Switches

### Kill Switch Implementation
```cpp
// Define kill switch configuration
inline const dynamic_config::Key<bool> kFeatureKillSwitch{
    "FEATURE_KILL_SWITCH", 
    false  // Default: feature enabled (kill switch off)
};

// Usage in application code
bool IsFeatureEnabled() const {
    const auto config = config_.GetSnapshot();
    return !config[kFeatureKillSwitch];  // Inverted logic
}
```

### Kill Switch Server Integration
```json
{
  "configs": {
    "FEATURE_KILL_SWITCH": true
  },
  "kill_switches_disabled": ["FEATURE_KILL_SWITCH"],
  "updated_at": "2024-01-01T12:00:00.000Z"
}
```

## Configuration Defaults and Fallbacks

### Default Configuration Management
```cpp
// Print all default configurations
// Command line: --print-dynamic-config-defaults

// Override defaults in static configuration
dynamic-config:
  defaults:
    MY_CONFIG_KEY: 100
    ANOTHER_CONFIG:
      enabled: true
      timeout_ms: 5000
```

### Fallback Mechanisms
1. **Runtime Configuration**: From configuration service
2. **Cache File**: Previously cached configuration
3. **Static Overrides**: From `dynamic-config.defaults`
4. **Code Defaults**: From `dynamic_config::Key` definitions

### Configuration Cache
```yaml
# Configuration caching setup
dynamic-config:
  fs-cache-path: /var/cache/service/config.json
  fs-task-processor: fs-task-processor
```

## Testing Patterns

### Unit Testing
```cpp
#include <userver/dynamic_config/test_helpers.hpp>

TEST(ComponentTest, DynamicConfiguration) {
    // Create test configuration storage
    auto storage = dynamic_config::MakeDefaultStorage({
        {kMyConfig, 100},
        {kFeatureEnabled, true}
    });
    
    auto source = storage.GetSource();
    auto snapshot = source.GetSnapshot();
    
    EXPECT_EQ(snapshot[kMyConfig], 100);
    EXPECT_TRUE(snapshot[kFeatureEnabled]);
}
```

### Testsuite Integration
```python
# Global configuration override
@pytest.mark.config(
    MY_CONFIG_KEY=42,
    FEATURE_ENABLED=True,
)
async def test_with_config(service_client):
    # Test with specific configuration
    pass

# Mid-test configuration changes
async def test_config_changes(service_client, dynamic_config):
    # Change configuration during test
    dynamic_config.set_values({
        'MY_CONFIG_KEY': 100,
        'FEATURE_ENABLED': False,
    })
    await service_client.update_server_state()
    
    # Test with new configuration
    response = await service_client.get('/api/endpoint')
    assert response.status == 200
```

### Kill Switch Testing
```python
from pytest_userver.plugins import dynamic_config as dynamic_config_lib

@pytest.mark.config(
    FEATURE_KILL_SWITCH=dynamic_config_lib.USE_STATIC_DEFAULT,
)
async def test_kill_switch_disabled(service_client):
    # Test with kill switch using static default
    pass
```

## Configuration Server Schema

### Request/Response Format
```json
// Request to configuration server
{
  "service": "my-service",
  "stage_name": "production",
  "ids": ["CONFIG_KEY_1", "CONFIG_KEY_2"],
  "updated_since": "2024-01-01T12:00:00.000Z"
}

// Response from configuration server
{
  "configs": {
    "CONFIG_KEY_1": {"enabled": true, "timeout": 1000},
    "CONFIG_KEY_2": "some_string_value"
  },
  "kill_switches_disabled": ["EMERGENCY_KILL_SWITCH"],
  "updated_at": "2024-01-01T12:30:00.000Z",
  "removed": []
}
```

## Best Practices

### Configuration Design
- **Use Descriptive Names**: Choose clear, UPPER_CASE configuration names
- **Provide Meaningful Defaults**: Ensure defaults work in production
- **Document Configuration**: Include descriptions in configuration schemas
- **Version Configuration**: Track configuration changes over time

### Performance Considerations
```cpp
// Store dynamic_config::Source, not Snapshot
class Component {
private:
    dynamic_config::Source config_;  // Good: lightweight reference
    // dynamic_config::Snapshot snapshot_;  // Bad: heavy, becomes stale
};

// Get snapshots when needed
void ProcessRequest() {
    const auto config = config_.GetSnapshot();  // Fresh configuration
    const auto timeout = config[kTimeoutConfig];
    // Use configuration for this request
}
```

### Error Handling
```cpp
// Configuration parsing with validation
MyConfig Parse(const formats::json::Value& value,
               formats::parse::To<MyConfig>) {
    auto config = MyConfig{
        value["timeout_ms"].As<int>(),
        value["retries"].As<int>()
    };
    
    // Validate configuration constraints
    if (config.timeout_ms <= 0) {
        throw std::runtime_error("timeout_ms must be positive");
    }
    if (config.retries < 0 || config.retries > 10) {
        throw std::runtime_error("retries must be between 0 and 10");
    }
    
    return config;
}
```

### Security Considerations
- **Validate Input**: Always validate configuration values
- **Audit Changes**: Log configuration updates for security auditing
- **Access Control**: Restrict configuration server access
- **Sensitive Data**: Avoid storing secrets in dynamic configuration

## Monitoring and Observability

### Configuration Metrics
```cpp
// Monitor configuration update health
// Metrics available:
// - dynamic-config.parse-errors
// - dynamic-config.was-last-parse-successful
// - cache.any.time.time-from-last-successful-start-ms
```

### Configuration Alerts
- **Parse Errors**: Alert on configuration parsing failures
- **Update Failures**: Monitor configuration service connectivity
- **Kill Switch Usage**: Track emergency kill switch activations
- **Configuration Drift**: Monitor differences between environments

## Cross-References

- **Static Configuration**: [`static-config.md`](./static-config.md) - Static configuration patterns
- **Environment Handling**: [`environment-handling.md`](./environment-handling.md) - Multi-environment configuration
- **Validation Patterns**: [`validation-patterns.md`](./validation-patterns.md) - Configuration validation strategies
- **Component System**: [`../../memory-bank/main/component-system.md`](../../memory-bank/main/component-system.md) - Component architecture
- **Service Patterns**: [`../../memory-bank/main/service-patterns.md`](../../memory-bank/main/service-patterns.md) - Service design patterns