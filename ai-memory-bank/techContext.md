# Technology Context

## Core Technologies

### Programming Language
- **C++17/20**: Primary language for framework and service implementation
- **Coroutines**: Leveraged for asynchronous programming model
- **STL**: Standard Template Library for common data structures and algorithms

### Build System
- **CMake**: Primary build system with comprehensive configuration
- **Makefile**: Simplified build interface for common operations
- **Conan**: Package manager for dependency management
- **CMakePresets.json**: Standardized build configurations

### Dependencies
- **Boost**: Utility libraries for system operations
- **fmt**: Modern formatting library
- **spdlog**: Fast logging library
- **OpenSSL**: Cryptographic operations and TLS support
- **libcurl**: HTTP client operations
- **Protocol Buffers**: Serialization for gRPC and other protocols
- **RapidJSON**: JSON parsing and generation
- **yaml-cpp**: YAML parsing and generation
- **Prometheus Client**: Metrics collection and exposition

## Database Technologies

### Supported Databases
- **PostgreSQL**: Primary relational database with async driver
- **MongoDB**: Document database with async driver
- **Redis/Valkey**: In-memory data structure store
- **ClickHouse**: Column-oriented database for analytics
- **MySQL/MariaDB**: Alternative relational database
- **YDB**: Distributed database system
- **SQLite**: Embedded database for local storage

### Database Patterns
- **Connection Pooling**: Efficient reuse of database connections
- **Prepared Statements**: Optimized query execution
- **Transaction Management**: ACID-compliant transaction support
- **Async Operations**: Non-blocking database interactions

## Communication Technologies

### HTTP Stack
- **HTTP/1.1**: Standard web protocol support
- **HTTP/2**: Modern protocol with multiplexing support
- **WebSocket**: Bidirectional communication protocol
- **HTTPS/TLS**: Secure communication support

### RPC Frameworks
- **gRPC**: High-performance RPC framework with protobuf
- **Generic HTTP Clients**: RESTful API integration

### Messaging Systems
- **Kafka**: Distributed streaming platform
- **RabbitMQ**: Message broker with AMQP support
- **TCP/UDP**: Low-level network communication

## Development Tools

### Testing Frameworks
- **GTest**: Unit testing framework
- **pytest**: Python-based functional testing
- **Google Benchmark**: Performance benchmarking
- **Chaos Testing**: Built-in fault injection framework

### Code Generation
- **Chaotic**: Data structure code generation from schemas
- **Chaotic OpenAPI**: Client and server code generation from OpenAPI specifications
- **Protocol Buffers**: Code generation for gRPC services

### Development Environment
- **Dev Containers**: Standardized development environments
- **Docker**: Containerization for consistent deployments
- **CI/CD**: Automated testing and deployment pipelines

## Observability Stack

### Logging
- **Structured Logging**: JSON-formatted log entries
- **Log Levels**: Trace, debug, info, warning, error, critical
- **Contextual Information**: Correlation IDs and request context

### Metrics
- **Prometheus**: Metrics collection and storage
- **Custom Metrics**: Business-specific KPI tracking
- **Histograms/Summaries**: Latency and distribution metrics

### Tracing
- **OpenTelemetry/Jaeger**: Distributed tracing implementation
- **Span Context**: Request flow tracking across services
- **Trace Attributes**: Business context in traces

## Development Setup

### Prerequisites
1. **C++ Compiler**: GCC 9+ or Clang 10+
2. **CMake**: 3.15+
3. **Python**: 3.8+ for build scripts and testing
4. **Conan**: For dependency management
5. **Docker**: For containerized development (optional)

### Build Process
```bash
# Clone repository
git clone https://github.com/userver-framework/userver.git

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
make -j$(nproc)

# Run tests
ctest
```

### Service Template
The `service_template/` directory provides a starting point for new services:
- Pre-configured CMakeLists.txt
- Sample configuration files
- Basic handler implementation
- Testing setup with pytest and GTest

### Development Workflow
1. **Create Service**: Copy and customize service_template
2. **Implement Handlers**: Add business logic to handler components
3. **Configure Components**: Update YAML configuration files
4. **Add Tests**: Implement unit and functional tests
5. **Build and Deploy**: Compile and deploy service

## Quality Assurance

### Code Review Process
- **Pull Requests**: All changes require review
- **Component Ownership**: Assign reviewers based on component expertise
- **Quality Tiers**: Platinum, Golden, and Silver quality standards

### Testing Strategies
- **Unit Tests**: Component-level testing with GTest
- **Functional Tests**: Service-level testing with pytest
- **Integration Tests**: Multi-component testing scenarios
- **Performance Tests**: Load and stress testing with benchmarks
- **Chaos Tests**: Fault injection and resilience testing

### Static Analysis
- **Clang-Tidy**: Static code analysis
- **Clang-Format**: Code formatting enforcement
- **Compiler Warnings**: Strict warning levels for code quality

## Deployment Options

### Containerization
- **Docker Images**: Pre-built images for all components
- **Multi-stage Builds**: Efficient image construction
- **Base Images**: Alpine Linux for minimal footprint

### Orchestration
- **Kubernetes**: Container orchestration support
- **Service Discovery**: Automatic service registration
- **Load Balancing**: Traffic distribution across instances

### Configuration Management
- **Environment Variables**: Runtime configuration
- **Configuration Files**: YAML-based configuration
- **Dynamic Configuration**: Runtime updates without restarts