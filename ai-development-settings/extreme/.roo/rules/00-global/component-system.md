# Component System - Global Rules

**Rule ID**: `global.component.system`  
**Priority**: 1000  
**Scope**: framework-wide  
**Override**: false  
**Extends**: [`global.framework.fundamentals`](framework-fundamentals.md)

## Component Lifecycle Management

### Mandatory Component Structure
Every component must follow the standard lifecycle:

```cpp
class MyComponent final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "my-component";
    
    // Constructor: Initialize component with dependencies
    MyComponent(const userver::components::ComponentConfig& config,
                const userver::components::ComponentContext& context);
    
    // OnAllComponentsLoaded: Called after all components are constructed
    void OnAllComponentsLoaded() override;
    
    // OnAllComponentsAreStopping: Called before components start stopping
    void OnAllComponentsAreStopping() override;
    
    // Destructor: Cleanup resources
    ~MyComponent() override;
    
private:
    // Component state and dependencies
};
```

**Memory Bank Reference**: [`memory-bank://main/component-system#lifecycle`](../memory-bank/main/component-system.md#lifecycle)

### Component Registration
Components must be registered in the component list:

```cpp
// In main.cpp
auto component_list = userver::components::MinimalServerComponentList()
    .Append<userver::components::Postgres>("postgres-db")
    .Append<userver::components::Redis>("redis")
    .Append<MyComponent>()
    .Append<MyHandler>();

return userver::utils::DaemonMain(argc, argv, component_list);
```

**Cross-Reference**: [`pattern://service_structure/component_registration`](../memory-bank/main/service-patterns.md#component-registration)

## Dependency Injection Patterns

### Constructor Dependency Injection
Declare all dependencies in constructor:

```cpp
MyHandler(const userver::components::ComponentConfig& config,
          const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      // Required dependencies
      pg_cluster_(context.FindComponent<userver::components::Postgres>("postgres-db")),
      redis_client_(context.FindComponent<userver::components::Redis>("redis")),
      // Optional dependencies with fallback
      metrics_storage_(context.FindComponentOptional<userver::components::StatisticsStorage>()) {
    
    // Validate required dependencies
    if (!pg_cluster_) {
        throw std::runtime_error("PostgreSQL component not found");
    }
}
```

### Dependency Resolution Rules
1. **Required Dependencies**: Use `FindComponent<T>()` - throws if not found
2. **Optional Dependencies**: Use `FindComponentOptional<T>()` - returns nullptr if not found
3. **Named Components**: Specify component name as second parameter
4. **Circular Dependencies**: Not allowed - will cause startup failure

**Memory Bank Reference**: [`memory-bank://main/component-system#dependency-injection`](../memory-bank/main/component-system.md#dependency-injection)

## Component Configuration

### Configuration Access
Access component configuration through `GetComponentConfig()`:

```cpp
MyComponent(const userver::components::ComponentConfig& config,
            const userver::components::ComponentContext& context)
    : ComponentBase(config, context) {
    
    // Parse configuration
    auto timeout = config["timeout"].As<std::chrono::milliseconds>(
        std::chrono::milliseconds{1000}  // default value
    );
    
    auto endpoint = config["endpoint"].As<std::string>();  // required
    auto max_connections = config["max_connections"].As<int>(10);  // optional with default
}
```

### Configuration Schema
Define configuration in `static_config.yaml`:

```yaml
components:
    my-component:
        timeout: 5000ms
        endpoint: "https://api.example.com"
        max_connections: 20
        # Optional nested configuration
        database:
            host: localhost
            port: 5432
```

**Cross-Reference**: [`concept://configuration/static_config`](../memory-bank/main/framework-core.md#static-configuration)

## Component Communication Patterns

### Service Locator Pattern
Components communicate through the component context:

```cpp
class ServiceA final : public userver::components::ComponentBase {
public:
    void DoSomething() {
        // Access other components through context
        auto& service_b = GetComponentContext().FindComponent<ServiceB>();
        service_b.ProcessData(data);
    }
};
```

### Event-Driven Communication
Use engine tasks for asynchronous communication:

```cpp
class EventPublisher final : public userver::components::ComponentBase {
private:
    void PublishEvent(const Event& event) {
        // Publish event asynchronously
        userver::engine::AsyncNoSpan([this, event]() {
            NotifySubscribers(event);
        }).Detach();
    }
};
```

**Memory Bank Reference**: [`memory-bank://main/async-programming#task-management`](../memory-bank/main/async-programming.md#task-management)

## Component State Management

### Thread Safety Requirements
Components must be thread-safe after construction:

```cpp
class ThreadSafeComponent final : public userver::components::ComponentBase {
private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, Data> cache_;
    
public:
    Data GetData(const std::string& key) const {
        std::shared_lock lock(mutex_);
        auto it = cache_.find(key);
        return it != cache_.end() ? it->second : Data{};
    }
    
    void SetData(const std::string& key, const Data& data) {
        std::unique_lock lock(mutex_);
        cache_[key] = data;
    }
};
```

### Resource Management
Follow RAII principles for resource cleanup:

```cpp
class ResourceManager final : public userver::components::ComponentBase {
private:
    std::unique_ptr<ExternalResource> resource_;
    
public:
    ResourceManager(const userver::components::ComponentConfig& config,
                   const userver::components::ComponentContext& context)
        : ComponentBase(config, context),
          resource_(std::make_unique<ExternalResource>(config["endpoint"].As<std::string>())) {
    }
    
    ~ResourceManager() override {
        // RAII ensures proper cleanup
        // resource_ is automatically destroyed
    }
};
```

## Component Testing Patterns

### Unit Testing Components
Test components in isolation:

```cpp
TEST(MyComponentTest, BasicFunctionality) {
    auto config = userver::components::ComponentConfig{};
    auto context = userver::components::ComponentContext{};
    
    // Mock dependencies
    auto mock_db = std::make_shared<MockDatabase>();
    context.Emplace<MockDatabase>(mock_db);
    
    // Test component
    MyComponent component(config, context);
    EXPECT_TRUE(component.IsHealthy());
}
```

### Integration Testing
Test component interactions:

```cpp
TEST(ComponentIntegrationTest, DatabaseInteraction) {
    // Use TestsuiteSupport for integration tests
    auto component_list = userver::components::MinimalServerComponentList()
        .Append<userver::components::TestsuiteSupport>()
        .Append<userver::components::Postgres>("postgres-test")
        .Append<MyComponent>();
    
    // Test with real database
    RunWithComponents(component_list, [](const auto& context) {
        auto& component = context.FindComponent<MyComponent>();
        EXPECT_TRUE(component.ProcessData("test"));
    });
}
```

**Memory Bank Reference**: [`memory-bank://main/service-patterns#testing`](../memory-bank/main/service-patterns.md#testing)

## Performance Considerations

### Component Initialization Order
Components are initialized in dependency order:

```cpp
// Dependencies are resolved automatically
// PostgreSQL component initializes before MyHandler
auto component_list = userver::components::MinimalServerComponentList()
    .Append<userver::components::Postgres>("postgres-db")  // Initializes first
    .Append<MyHandler>();  // Initializes after postgres-db
```

### Lazy Initialization
Use lazy initialization for expensive resources:

```cpp
class LazyComponent final : public userver::components::ComponentBase {
private:
    mutable std::once_flag init_flag_;
    mutable std::unique_ptr<ExpensiveResource> resource_;
    
    void InitializeResource() const {
        std::call_once(init_flag_, [this]() {
            resource_ = std::make_unique<ExpensiveResource>();
        });
    }
    
public:
    const ExpensiveResource& GetResource() const {
        InitializeResource();
        return *resource_;
    }
};
```

## Error Handling in Components

### Constructor Error Handling
Handle initialization errors properly:

```cpp
MyComponent(const userver::components::ComponentConfig& config,
            const userver::components::ComponentContext& context)
    : ComponentBase(config, context) {
    
    try {
        InitializeResource(config["endpoint"].As<std::string>());
    } catch (const std::exception& e) {
        LOG_ERROR() << "Failed to initialize component: " << e.what();
        throw;  // Re-throw to prevent service startup
    }
}
```

### Runtime Error Handling
Handle runtime errors gracefully:

```cpp
void ProcessRequest(const Request& request) {
    try {
        DoProcessing(request);
    } catch (const TransientError& e) {
        LOG_WARNING() << "Transient error, will retry: " << e.what();
        // Implement retry logic
    } catch (const PermanentError& e) {
        LOG_ERROR() << "Permanent error: " << e.what();
        throw;  // Propagate to caller
    }
}
```

## Cross-References

### Related Memory Bank Entries
- [`memory-bank://main/component-system`](../memory-bank/main/component-system.md) - Detailed component patterns
- [`memory-bank://main/framework-core#components`](../memory-bank/main/framework-core.md#components) - Core component concepts
- [`memory-bank://main/service-patterns#architecture`](../memory-bank/main/service-patterns.md#architecture) - Service architecture patterns

### Implementation Examples
- [`example://component/basic`](../../samples/hello_service/src/hello.cpp) - Basic component implementation
- [`example://component/database`](../../samples/postgres_service/) - Database component integration
- [`example://component/testing`](../../samples/hello_service/unittests/) - Component testing patterns

### Alternative Approaches
- [`alternative://singleton_vs_component`](../memory-bank/main/component-system.md#design-patterns) - Component vs singleton patterns
- [`alternative://service_locator_vs_di`](../memory-bank/research/new-patterns.md#dependency-injection) - Service locator alternatives

---

**Inheritance**: Extends framework fundamentals with component-specific rules.  
**Dependencies**: [`global.framework.fundamentals`](framework-fundamentals.md)  
**Validation**: All components must follow these patterns.  
**Last Updated**: 2025-01-05  
**Next Review**: 2025-04-05