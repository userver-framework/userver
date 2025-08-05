# userver Service Development Rules

service_development_rules:
  description: "Rules and guidelines for developing services using the userver framework"
  
  # Notes about available resources
  notes:
    samples: "In ~/userver/samples there are many examples of userver services demonstrating various features and components"
    framework_source: "In ~/userver there is the source code of the framework for reference"
    memory_bank: "In ~/userver/ai-memory-bank there is memory about framework sources and patterns"
  
  # Service project structure based on service_template
  project_structure:
    description: "Standard directory structure for userver services"
    structure:
      service_name:
        - "CMakeLists.txt              # Build configuration"
        - "configs/                    # Configuration files"
        - "  └── static_config.yaml    # Static service configuration"
        - "src/                        # Source code"
        - "  ├── main.cpp              # Service entry point"
        - "  ├── handlers/             # HTTP handlers"
        - "  │   ├── handler_name.hpp  # Handler declaration"
        - "  │   └── handler_name.cpp  # Handler implementation"
        - "  └── components/           # Custom components (optional)"
        - "tests/                      # Tests (optional)"
        - "unittests/                  # Unit tests (optional)"
        - "benchmarks/                 # Benchmarks (optional)"
  
  # Key implementation files
  key_files:
    main_cpp:
      description: "Service entry point"
      responsibilities:
        - "Sets up the component list"
        - "Registers handlers and components"
        - "Starts the service using userver::utils::DaemonMain"
    
    cmake_lists_txt:
      description: "Build configuration"
      responsibilities:
        - "Finds the userver package"
        - "Defines object libraries for source files"
        - "Links against userver components"
        - "Sets up installation targets"
    
    static_config_yaml:
      description: "Configuration file"
      responsibilities:
        - "Defines task processors"
        - "Configures components"
        - "Maps handlers to URLs"
  
  # Service configuration patterns
  configuration_patterns:
    handler_configuration:
      description: "Handlers are configured in static_config.yaml"
      pattern: |
        components:
            handler-name:
                path: /url-path
                method: GET,POST  # HTTP methods
                task_processor: main-task-processor
    
    task_processor_configuration:
      description: "Services typically define at least two task processors"
      pattern: |
        task_processors:
            main-task-processor:
                worker_threads: 4  # For CPU-bound tasks
            fs-task-processor:
                worker_threads: 2  # For filesystem-bound tasks
    
    component_registration:
      description: "Components are registered in main.cpp using the component list"
      pattern: |
        auto component_list = userver::components::MinimalServerComponentList()
            .Append<userver::server::handlers::Ping>()
            .Append<service_namespace::HandlerName>();
  
  # Development workflows
  development_workflows:
    building:
      steps:
        - "CMake Configuration: Run cmake to configure the build"
        - "Compilation: Run make or ninja to build the service"
        - "Installation: Run make install to install the service"
    
    testing:
      steps:
        - "Unit Tests: Run with make test or ctest"
        - "Functional Tests: Use testsuite framework"
        - "Benchmarks: Run performance benchmarks"
    
    deployment:
      steps:
        - "Binary Deployment: Install the service binary"
        - "Configuration Deployment: Deploy configuration files"
        - "Service Management: Use systemd or similar for service management"
  
  # Service-specific rules and guidelines
  service_rules:
    handler_implementation:
      guidelines:
        - "Handler Classes: Inherit from userver::server::handlers::HttpHandlerBase"
        - "Handler Names: Use kName constant for component registration"
        - "Request Handling: Implement HandleRequestThrow method for exception-based error handling"
    
    component_design:
      guidelines:
        - "Component Lifecycle: Follow userver component lifecycle (Constructor → Start → Stop → Destructor)"
        - "Configuration: Use GetComponentConfig() to access component configuration"
        - "Dependencies: Declare dependencies in constructor parameters"
    
    error_handling:
      guidelines:
        - "Exceptions: Use exceptions for error handling (they are automatically converted to HTTP error responses)"
        - "Logging: Use userver::logging for structured logging"
        - "Validation: Validate input parameters and return appropriate HTTP status codes"
  
  # Key components that services typically use
  key_components:
    core_components:
      - "server: HTTP server component"
      - "logging: Logging component"
      - "http-client: HTTP client for making outgoing requests"
      - "dns-client: DNS client component"
      - "handler-ping: Health check endpoint"
      - "tests-control: Testsuite control endpoint"
    
    database_components:
      description: "Depending on service requirements"
      components:
        - "postgres: PostgreSQL database component"
        - "mongo: MongoDB database component"
        - "redis: Redis/Valkey database component"
        - "mysql: MySQL database component"
    
    optional_components:
      - "testsuite-support: Testsuite support component"
      - "congestion-control: Congestion control component"
      - "dynamic-config: Dynamic configuration component"
  
  # Patterns for HTTP handlers, middleware, and error handling
  implementation_patterns:
    http_handler_patterns:
      simple_handler: |
        class MyHandler final : public userver::server::handlers::HttpHandlerBase {
        public:
            static constexpr std::string_view kName = "handler-my-handler";
            
            std::string HandleRequestThrow(
                const userver::server::http::HttpRequest& request,
                userver::server::request::RequestContext&) const override;
        };
      
      request_processing:
        - "Extract parameters from query string: request.GetArg(\"name\")"
        - "Extract headers: request.GetHeader(\"header-name\")"
        - "Set response headers: request.GetHttpResponse().SetHeader(\"header-name\", \"value\")"
        - "Set response content type: request.GetHttpResponse().SetContentType(\"text/plain\")"
    
    middleware_patterns:
      patterns:
        - "Component-Based Middleware: Implement as components that wrap handlers"
        - "Handler Chaining: Use handler composition for cross-cutting concerns"
        - "Request Context: Use RequestContext to pass data between middleware components"
    
    error_handling_patterns:
      patterns:
        - "Exception-Based: Throw exceptions for error conditions"
        - "HTTP Status Codes: Return appropriate HTTP status codes"
        - "Structured Logging: Log errors with context information"
      
      exception_example: |
        if (some_error_condition) {
            throw userver::server::handlers::ClientError(
                userver::server::handlers::InternalError, "Error message");
        }
    
    best_practices:
      - "Asynchronous Operations: Use userver's asynchronous drivers for I/O operations"
      - "Deadline Propagation: Propagate deadlines to downstream services"
      - "Circuit Breakers: Use circuit breaker patterns for external dependencies"
      - "Caching: Implement caching for frequently accessed data"
      - "Rate Limiting: Implement rate limiting to protect services"