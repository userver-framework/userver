# Environment-Specific Configuration Handling

## Overview

Environment-specific configuration management in userver enables seamless deployment across development, staging, and production environments while maintaining security and consistency. This system supports environment variable integration, configuration inheritance, and deployment-specific overrides.

## Core Principles

### Multi-Environment Strategy
- **Environment Separation**: Clear separation between dev, staging, and production configurations
- **Configuration Inheritance**: Base configurations with environment-specific overrides
- **Security Boundaries**: Different security modes for different environments
- **Deployment Flexibility**: Support for various deployment scenarios

### Environment Variable Integration
```cpp
// Environment variable access modes
yaml_config::YamlConfig::Mode::kSecure           // Production: no env access
yaml_config::YamlConfig::Mode::kEnvAllowed       // Staging: env variables only
yaml_config::YamlConfig::Mode::kEnvAndFileAllowed // Development: full access
```

## Implementation Patterns

### Environment Variable Configuration
```yaml
# Environment-specific configuration with fallbacks
database:
  # Required environment variable
  host#env: DB_HOST
  host#fallback: "localhost"
  
  # Sensitive data from environment
  password#env: DB_PASSWORD
  password#fallback: ""  # Empty fallback for required secrets
  
  # Optional environment overrides
  pool_size#env: DB_POOL_SIZE
  pool_size#fallback: "10"
  
  # Complex environment configuration
  connection_string#env: DATABASE_URL
  connection_string#fallback: "postgresql://localhost:5432/mydb"
```

### File-Based Environment Configuration
```yaml
# File inclusion for environment-specific settings
ssl_config#file: /etc/ssl/config.yaml
ssl_config#fallback:
  enabled: false
  cert_path: ""
  key_path: ""

# Environment-specific service discovery
service_discovery#file: $SERVICE_DISCOVERY_CONFIG
service_discovery#fallback:
  enabled: false
  endpoints: []
```

### Configuration Composition
```yaml
# Base configuration (config.yaml)
server:
  port: 8080
  threads: 4

database:
  pool_size: 10
  timeout: 30s

# Environment override (production.yaml)
database:
  pool_size#env: PROD_DB_POOL_SIZE
  pool_size#fallback: "50"
  
logging:
  level#env: LOG_LEVEL
  level#fallback: "info"
```

## Environment-Specific Patterns

### Development Environment
```yaml
# Development configuration (permissive mode)
components_manager:
  components:
    dynamic-config:
      defaults:
        LOG_LEVEL: "debug"
        ENABLE_DEBUG_ENDPOINTS: true
        
    server:
      listener:
        port#env: DEV_PORT
        port#fallback: 8080
        
    postgres-db:
      dbconnection#env: DEV_DATABASE_URL
      dbconnection#fallback: "postgresql://localhost:5432/mydb_dev"
```

### Staging Environment
```yaml
# Staging configuration (controlled environment access)
components_manager:
  components:
    dynamic-config:
      defaults:
        LOG_LEVEL: "info"
        ENABLE_DEBUG_ENDPOINTS: false
        
    server:
      listener:
        port#env: STAGING_PORT
        port#fallback: 8080
        
    postgres-db:
      dbconnection#env: STAGING_DATABASE_URL
      # No fallback for staging - must be explicitly configured
```

### Production Environment
```yaml
# Production configuration (secure mode)
components_manager:
  components:
    dynamic-config:
      defaults:
        LOG_LEVEL: "warning"
        ENABLE_DEBUG_ENDPOINTS: false
        
    server:
      listener:
        port: 8080  # Fixed port, no environment override
        
    postgres-db:
      dbconnection: $database_connection_string  # From config_vars only
```

## Configuration Security Modes

### Secure Mode (Production)
```cpp
// Production: no environment or file access
const yaml_config::YamlConfig config(
    yaml_data, 
    config_vars,
    yaml_config::YamlConfig::Mode::kSecure
);
```

### Environment-Allowed Mode (Staging)
```cpp
// Staging: environment variables allowed
const yaml_config::YamlConfig config(
    yaml_data, 
    config_vars,
    yaml_config::YamlConfig::Mode::kEnvAllowed
);
```

### Full Access Mode (Development)
```cpp
// Development: environment variables and files allowed
const yaml_config::YamlConfig config(
    yaml_data, 
    config_vars,
    yaml_config::YamlConfig::Mode::kEnvAndFileAllowed
);
```

## Deployment Strategies

### Container-Based Deployment
```dockerfile
# Dockerfile with environment-specific configuration
FROM userver-base

# Copy base configuration
COPY config/ /etc/service/

# Environment-specific configuration via environment variables
ENV DB_HOST=production-db.example.com
ENV DB_POOL_SIZE=50
ENV LOG_LEVEL=info

# Configuration validation at startup
RUN /usr/local/bin/validate-config --config /etc/service/config.yaml
```

### Kubernetes Deployment
```yaml
# Kubernetes ConfigMap for environment-specific settings
apiVersion: v1
kind: ConfigMap
metadata:
  name: service-config
data:
  config.yaml: |
    server:
      port#env: SERVICE_PORT
      port#fallback: "8080"
    database:
      host#env: DB_HOST
      pool_size#env: DB_POOL_SIZE
      pool_size#fallback: "10"

---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-service
spec:
  template:
    spec:
      containers:
      - name: service
        env:
        - name: SERVICE_PORT
          value: "8080"
        - name: DB_HOST
          value: "postgres-service"
        - name: DB_POOL_SIZE
          value: "20"
```

### Environment Variable Patterns
```bash
# Development environment
export DEV_MODE=true
export LOG_LEVEL=debug
export DB_HOST=localhost
export DB_POOL_SIZE=5

# Staging environment
export ENVIRONMENT=staging
export LOG_LEVEL=info
export DB_HOST=staging-db.internal
export DB_POOL_SIZE=15

# Production environment
export ENVIRONMENT=production
export LOG_LEVEL=warning
export DB_HOST=prod-db.internal
export DB_POOL_SIZE=50
```

## Configuration Validation by Environment

### Environment-Specific Validation
```cpp
class EnvironmentConfig {
public:
    enum class Environment { kDevelopment, kStaging, kProduction };
    
    static Environment GetEnvironment() {
        const auto env = std::getenv("ENVIRONMENT");
        if (!env) return Environment::kDevelopment;
        
        if (std::string(env) == "production") return Environment::kProduction;
        if (std::string(env) == "staging") return Environment::kStaging;
        return Environment::kDevelopment;
    }
    
    static yaml_config::YamlConfig::Mode GetConfigMode() {
        switch (GetEnvironment()) {
            case Environment::kProduction:
                return yaml_config::YamlConfig::Mode::kSecure;
            case Environment::kStaging:
                return yaml_config::YamlConfig::Mode::kEnvAllowed;
            case Environment::kDevelopment:
                return yaml_config::YamlConfig::Mode::kEnvAndFileAllowed;
        }
    }
};
```

### Configuration Schema Validation
```cpp
// Environment-specific schema validation
yaml_config::Schema GetEnvironmentSchema() {
    const auto env = EnvironmentConfig::GetEnvironment();
    
    std::string schema = R"(
type: object
properties:
  database:
    type: object
    properties:
      host:
        type: string
      pool_size:
        type: integer
        minimum: 1
)";

    if (env == EnvironmentConfig::Environment::kProduction) {
        schema += R"(
        maximum: 100
      required: ["host", "pool_size"]
)";
    }
    
    return yaml_config::Schema::FromString(schema);
}
```

## Testing Across Environments

### Environment-Specific Testing
```cpp
class EnvironmentTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Save original environment
        original_env_ = std::getenv("ENVIRONMENT");
    }
    
    void TearDown() override {
        // Restore original environment
        if (original_env_) {
            setenv("ENVIRONMENT", original_env_, 1);
        } else {
            unsetenv("ENVIRONMENT");
        }
    }
    
    void SetEnvironment(const std::string& env) {
        setenv("ENVIRONMENT", env.c_str(), 1);
    }

private:
    const char* original_env_;
};

TEST_F(EnvironmentTest, ProductionConfiguration) {
    SetEnvironment("production");
    
    // Test production-specific configuration behavior
    const auto config_mode = EnvironmentConfig::GetConfigMode();
    EXPECT_EQ(config_mode, yaml_config::YamlConfig::Mode::kSecure);
}
```

### Testsuite Environment Testing
```python
# Test environment-specific behavior
@pytest.mark.parametrize('environment,expected_log_level', [
    ('development', 'debug'),
    ('staging', 'info'),
    ('production', 'warning'),
])
async def test_environment_configuration(
    service_client, environment, expected_log_level, monkeypatch
):
    monkeypatch.setenv('ENVIRONMENT', environment)
    
    # Restart service with new environment
    await service_client.update_server_state()
    
    response = await service_client.get('/debug/log-level')
    assert response.json()['level'] == expected_log_level
```

## Configuration Migration Patterns

### Environment Migration
```yaml
# Migration from environment variables to dynamic config
# Old: environment variable
database:
  timeout#env: DB_TIMEOUT
  timeout#fallback: "30s"

# New: dynamic configuration with environment fallback
database:
  timeout: $db_timeout_config  # From dynamic config
  timeout#env: DB_TIMEOUT      # Fallback to environment
  timeout#fallback: "30s"     # Final fallback
```

### Gradual Migration Strategy
```cpp
// Support both old and new configuration methods
class DatabaseConfig {
public:
    static std::chrono::seconds GetTimeout(
        const yaml_config::YamlConfig& config,
        const dynamic_config::Snapshot& dynamic_config) {
        
        // Try dynamic configuration first
        if (dynamic_config.HasValue(kDatabaseTimeout)) {
            return dynamic_config[kDatabaseTimeout];
        }
        
        // Fall back to static configuration
        return config["database"]["timeout"].As<std::chrono::seconds>();
    }
};
```

## Best Practices

### Environment Design
- **Minimize Differences**: Keep environment configurations as similar as possible
- **Explicit Configuration**: Make environment-specific settings explicit
- **Security by Default**: Use most restrictive mode appropriate for environment
- **Validation**: Validate configuration in all environments

### Security Considerations
```yaml
# Secure environment variable handling
secrets:
  # Never use fallbacks for secrets in production
  api_key#env: API_KEY
  # api_key#fallback: ""  # Commented out for production
  
  # Use different variables per environment
  database_password#env: PROD_DB_PASSWORD  # Production
  # database_password#env: STAGING_DB_PASSWORD  # Staging
```

### Configuration Organization
```
config/
├── base/
│   ├── config.yaml          # Base configuration
│   └── components.yaml      # Component definitions
├── environments/
│   ├── development.yaml     # Development overrides
│   ├── staging.yaml         # Staging overrides
│   └── production.yaml      # Production overrides
└── secrets/
    ├── dev-secrets.yaml     # Development secrets
    ├── staging-secrets.yaml # Staging secrets
    └── prod-secrets.yaml    # Production secrets (encrypted)
```

### Deployment Validation
```bash
#!/bin/bash
# Pre-deployment configuration validation

ENVIRONMENT=${1:-development}

echo "Validating configuration for environment: $ENVIRONMENT"

# Validate configuration syntax
./validate-config --config config/base/config.yaml \
                  --overrides config/environments/$ENVIRONMENT.yaml

# Check required environment variables
case $ENVIRONMENT in
  production)
    required_vars="DB_HOST DB_PASSWORD API_KEY"
    ;;
  staging)
    required_vars="DB_HOST DB_PASSWORD"
    ;;
  development)
    required_vars=""
    ;;
esac

for var in $required_vars; do
  if [ -z "${!var}" ]; then
    echo "ERROR: Required environment variable $var is not set"
    exit 1
  fi
done

echo "Configuration validation passed"
```

## Monitoring and Observability

### Environment-Specific Monitoring
```cpp
// Environment-aware metrics
class EnvironmentMetrics {
public:
    static void RecordConfigurationLoad(
        const std::string& environment,
        bool success) {
        
        auto& metrics = utils::statistics::GetCurrentStatistics();
        metrics.GetMetric("config.load")
            .WithTag("environment", environment)
            .WithTag("success", success ? "true" : "false")
            .Increment();
    }
};
```

### Configuration Drift Detection
- **Environment Comparison**: Compare configurations across environments
- **Change Tracking**: Track configuration changes over time
- **Compliance Monitoring**: Ensure configurations meet security requirements
- **Audit Logging**: Log all configuration access and modifications

## Cross-References

- **Static Configuration**: [`static-config.md`](./static-config.md) - Static configuration patterns
- **Dynamic Configuration**: [`dynamic-config.md`](./dynamic-config.md) - Runtime configuration updates
- **Validation Patterns**: [`validation-patterns.md`](./validation-patterns.md) - Configuration validation strategies
- **Component System**: [`../../memory-bank/main/component-system.md`](../../memory-bank/main/component-system.md) - Component architecture
- **Service Patterns**: [`../../memory-bank/main/service-patterns.md`](../../memory-bank/main/service-patterns.md) - Service design patterns