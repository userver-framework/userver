# Static Configuration Management

## Overview

Static configuration in userver applications is managed through YAML files that define component settings, service parameters, and application behavior. This configuration is loaded at startup and provides the foundation for service initialization.

## Core Principles

### YAML Configuration Structure
- **Component-Based Organization**: Each component has its own configuration section
- **Hierarchical Structure**: Use nested YAML structures for complex configurations
- **Variable Substitution**: Support for `$variable` substitution from config_vars
- **Environment Integration**: Use `#env`, `#file`, and `#fallback` suffixes for dynamic values

### Configuration Modes
```cpp
// Security modes for configuration handling
yaml_config::YamlConfig::Mode::kSecure           // No environment access
yaml_config::YamlConfig::Mode::kEnvAllowed       // Environment variables allowed
yaml_config::YamlConfig::Mode::kEnvAndFileAllowed // Environment + file access
```

## Implementation Patterns

### Component Configuration
```cpp
// Component constructor with configuration
Component::Component(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : components::ComponentBase(config, context) {
    
    // Read static configuration values
    const auto url = config["some-url"].As<std::string>();
    const auto timeout = config["timeout"].As<std::chrono::seconds>();
    const auto enabled = config["enabled"].As<bool>(true); // with default
}
```

### YAML Configuration Access
```cpp
// Access configuration with type safety
auto value = config["key"].As<int>();
auto optional_value = config["optional-key"].As<std::optional<std::string>>();
auto with_default = config["key"].As<int>(42);

// Check for missing values
if (!config["key"].IsMissing()) {
    // Process configuration
}
```

### Variable Substitution
```yaml
# config_vars.yaml
database_host: "localhost"
database_port: 5432

# main config
database:
  host: $database_host
  port: $database_port
  connection_string: "postgresql://$database_host:$database_port/mydb"
```

### Environment Variable Integration
```yaml
# Environment variable with fallback
database:
  password#env: DB_PASSWORD
  password#fallback: "default_password"
  
# File-based configuration
ssl:
  certificate#file: /etc/ssl/cert.pem
  certificate#fallback: "default_cert_content"
```

## Configuration Organization

### Service Configuration Structure
```yaml
# Main service configuration
components_manager:
  components:
    # HTTP server configuration
    server:
      listener:
        port: 8080
        task_processor: main-task-processor
    
    # Database configuration
    postgres-db:
      dbconnection: $database_connection_string
      blocking_task_processor: fs-task-processor
    
    # Custom component configuration
    my-component:
      some-url: $external_service_url
      timeout: 30s
      enabled: true
```

### Component-Specific Patterns
```yaml
# HTTP client configuration
http-client:
  pool-statistics-disable: false
  thread-name-prefix: http-client
  threads: 2
  defer-events: false

# Cache configuration  
key-value-cache:
  size: 1000
  ways: 16
  lifetime: 10s
  config-updates-enabled: true
```

## Validation and Schema

### Static Configuration Validation
```cpp
// Define configuration schema for validation
yaml_config::Schema Component::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<components::ComponentBase>(R"(
type: object
description: My component configuration
additionalProperties: false
properties:
  some-url:
    type: string
    description: URL for external service
  timeout:
    type: string
    description: Request timeout duration
    default: "30s"
  enabled:
    type: boolean
    description: Enable component functionality
    default: true
)");
}
```

### Configuration File Mode
```cpp
// Specify component configuration requirements
template<>
inline constexpr auto components::kConfigFileMode<MyComponent> = 
    components::ConfigFileMode::kRequired;

// Optional configuration
template<>
inline constexpr auto components::kConfigFileMode<OptionalComponent> = 
    components::ConfigFileMode::kNotRequired;
```

## Best Practices

### Configuration Design
- **Use Descriptive Names**: Choose clear, self-documenting configuration keys
- **Provide Defaults**: Always specify reasonable default values
- **Group Related Settings**: Organize related configuration under common sections
- **Document Schema**: Use schema validation with descriptions

### Type Safety
```cpp
// Prefer strongly-typed configuration access
auto timeout = config["timeout"].As<std::chrono::seconds>();
auto port = config["port"].As<std::uint16_t>();
auto enabled = config["enabled"].As<bool>();

// Use optional for non-required values
auto optional_setting = config["optional"].As<std::optional<std::string>>();
```

### Environment Handling
```yaml
# Secure environment variable handling
database:
  # Use environment variables for sensitive data
  password#env: DB_PASSWORD
  password#fallback: ""  # Empty fallback for required secrets
  
  # Use fallbacks for optional configuration
  pool_size#env: DB_POOL_SIZE
  pool_size#fallback: "10"
```

### Configuration Validation
- **Enable Schema Validation**: Use `GetStaticConfigSchema()` for all components
- **Validate Early**: Catch configuration errors at startup
- **Provide Clear Messages**: Include helpful error descriptions
- **Test Configuration**: Validate configuration in unit tests

## Security Considerations

### Safe Configuration Modes
```cpp
// Use secure mode for production
yaml_config::YamlConfig config(yaml_data, config_vars, 
                              yaml_config::YamlConfig::Mode::kSecure);

// Only allow environment access when necessary
yaml_config::YamlConfig config(yaml_data, config_vars, 
                              yaml_config::YamlConfig::Mode::kEnvAllowed);
```

### Sensitive Data Handling
- **Never Log Secrets**: Avoid logging configuration containing passwords
- **Use Environment Variables**: Store secrets in environment variables
- **Validate Sources**: Only allow trusted configuration sources
- **Audit Access**: Monitor configuration access patterns

## Testing Patterns

### Configuration Testing
```cpp
// Test component with custom configuration
TEST(ComponentTest, CustomConfiguration) {
    const auto config_yaml = formats::yaml::FromString(R"(
some-url: "https://test.example.com"
timeout: "5s"
enabled: true
)");
    
    const components::ComponentConfig config{config_yaml, {}};
    // Test component initialization with config
}
```

### Mock Configuration
```cpp
// Create test configuration
auto CreateTestConfig() {
    return formats::yaml::FromString(R"(
components_manager:
  components:
    my-component:
      test-mode: true
      mock-responses: true
)");
}
```

## Cross-References

- **Dynamic Configuration**: [`dynamic-config.md`](./dynamic-config.md) - Runtime configuration updates
- **Environment Handling**: [`environment-handling.md`](./environment-handling.md) - Multi-environment configuration
- **Validation Patterns**: [`validation-patterns.md`](./validation-patterns.md) - Configuration validation strategies
- **Component System**: [`../../memory-bank/main/component-system.md`](../../memory-bank/main/component-system.md) - Component architecture
- **Service Patterns**: [`../../memory-bank/main/service-patterns.md`](../../memory-bank/main/service-patterns.md) - Service design patterns