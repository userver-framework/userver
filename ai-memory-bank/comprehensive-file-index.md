# Comprehensive File Index

## Root Directory
- .ai_system_prompt - AI system prompt configuration file
- .clang-format - Code formatting configuration for C++ code
- .clang-tidy - Static analysis configuration for C++ code
- .cmake-format.py - Configuration for formatting CMake files
- .editorconfig - Editor configuration for consistent coding styles across editors
- .gitattributes - Git attributes configuration for file handling
- .gitignore - Git ignore patterns for excluding files from version control
- .mapping.json - JSON mapping configuration file
- .piglet-meta.json - Metadata configuration for Piglet tooling
- 🛡️ .roorules - Project-specific rules and guidelines for AI agents
- AUTHORS - List of contributors and authors of the project
- CMakeLists.txt - Root CMake build configuration file
- CODE_OF_CONDUCT.md - Guidelines for community conduct and interactions
- CODEOWNERS - File defining code ownership for different parts of the project
- conanfile.py - Conan package manager configuration file
- CONTRIBUTING.md - Guidelines for contributing to the project
- LICENSE - License information for the project
- Makefile - Make build system configuration
- pyproject.toml - Python project configuration file
- pytest.ini - Configuration file for pytest testing framework
- README.md - Main project documentation and introduction
- SECURITY.md - Security policies and reporting procedures
- THIRD_PARTY.md - Information about third-party dependencies and licenses

### GitHub Configuration (.github/)
- dependabot.yml - Configuration for automated dependency updates
- docker-compose.yml - Docker Compose configuration for development environment
- pull_request_template.md - Template for pull request descriptions
- ISSUE_TEMPLATE/ - Templates for different types of issues
- workflows/ - GitHub Actions workflow definitions

## Core Framework (core/)
The core framework provides the foundation components for building high-performance C++ services with asynchronous I/O and coroutine support.

### Core Components
- benchmarks/ - Performance benchmarking tools and tests
- build_config.hpp.in - Build configuration template file
- CMakeLists.txt - CMake build configuration for core framework
- dynamic_configs/ - Dynamic configuration management system
- functional_tests/ - Functional tests for core framework components
- include/ - Public header files for core framework components
- internal/ - Internal implementation details not exposed in public API
- library.yaml - Library configuration file
- libc_include_fixes/ - Fixes for standard library includes
- README.md - Core framework documentation
- src/ - Source code implementation of core framework components
- static_configs/ - Static configuration files
- sys_coro/ - System coroutine implementation
- uboost_coro/ - Boost coroutine integration
- utest/ - Unit testing framework and utilities

### Core Subdirectories
#### alerts/
Framework for handling alert notifications and monitoring
- source.hpp - Alert source interface

#### baggage/
Distributed tracing baggage propagation implementation
- baggage_manager.hpp - Baggage manager implementation
- baggage.hpp - Baggage data structure
- fwd.hpp - Forward declarations for baggage components

#### cache/
Caching framework for in-memory and external cache solutions
- cache_config.hpp - Cache configuration structures
- cache_statistics.hpp - Cache statistics tracking
- caching_component_base.hpp - Base class for caching components
- expirable_lru_cache.hpp - LRU cache with expiration support
- lru_cache_component_base.hpp - Base class for LRU cache components
- lru_cache_config.hpp - LRU cache configuration
- lru_cache_statistics.hpp - LRU cache statistics
- nway_lru_cache.hpp - N-way LRU cache implementation
- update_type.hpp - Cache update type definitions

#### clients/
HTTP and other client implementations for external service communication

##### clients/dns/
DNS client implementation
- component.hpp - DNS client component
- resolver.hpp - DNS resolver implementation

##### clients/http/
HTTP client implementation
- client.hpp - HTTP client interface
- component.hpp - HTTP client component
- request.hpp - HTTP request representation
- response.hpp - HTTP response representation
- plugins/ - HTTP client plugins for additional functionality

#### components/
Component system for modular service architecture with lifecycle management
- component_base.hpp - Base class for all components
- component_list.hpp - Component list management
- run.hpp - Service startup and shutdown functions

#### concurrent/
Concurrency primitives and utilities for thread-safe operations

#### congestion_control/
Network congestion control mechanisms
- component.hpp - Congestion control component
- controller.hpp - Congestion controller interface
- limiter.hpp - Request rate limiter
- sensor.hpp - Congestion monitoring sensor

#### dist_lock/
Distributed locking implementation for coordination between services

#### dump/
Data dump and restore functionality for caching components
- config.hpp - Dump configuration structures
- dumper.hpp - Data dumper interface
- operations.hpp - Dump operations

#### dynamic_config/
Dynamic configuration system for runtime configuration changes
- snapshot.hpp - Configuration snapshot
- source.hpp - Configuration source
- value.hpp - Typed configuration value wrapper

#### engine/
Coroutine-based engine for asynchronous I/O operations
- async.hpp - Asynchronous task execution
- task.hpp - Task management
- mutex.hpp - Coroutine-aware mutex
- condition_variable.hpp - Coroutine-aware condition variable

#### fs/
File system operations and utilities

#### logging/
Structured logging framework with multiple output options

#### middlewares/
Middleware components for request/response processing
- pipeline.hpp - Middleware pipeline implementation
- runner.hpp - Middleware execution runner

#### net/
Networking utilities and implementations

#### os_signals/
Operating system signal handling
- component.hpp - OS signals handling component
- processor.hpp - Signal processor implementation

#### rcu/
Read-Copy-Update (RCU) synchronization mechanism
- rcu.hpp - RCU implementation
- rcu_map.hpp - RCU-protected map data structure

#### server/
HTTP server implementation and related components

#### storages/
Storage abstractions for various database systems

#### tracing/
Distributed tracing implementation

#### utils/
General utility functions and classes

## Code Generation Components

### Chaotic (chaotic/)
Code generation framework for data structures and serialization

#### Chaotic Directories
- bin/ - Executable scripts for code generation
- bin-dynamic-configs/ - Dynamic configuration generation scripts
- chaotic/ - Core chaotic implementation
- golden_tests/ - Reference tests for generated code
- include/ - Public header files for chaotic components
- integration_tests/ - Integration tests for chaotic components
- mypy/ - Python type checking configuration
- src/ - Source code implementation
- tests/ - Unit tests for chaotic components

### Chaotic OpenAPI (chaotic-openapi/)
OpenAPI-based code generation for client and server implementations

#### Chaotic OpenAPI Directories
- bin/ - Executable scripts for OpenAPI code generation
- chaotic_openapi/ - Core OpenAPI implementation
- golden_tests/ - Reference tests for generated OpenAPI code
- include/ - Public header files for OpenAPI components
- integration_tests/ - Integration tests for OpenAPI components
- mypy/ - Python type checking configuration
- src/ - Source code implementation
- tests/ - Unit tests for OpenAPI components

## Database Components

### ClickHouse (clickhouse/)
ClickHouse database driver and components for analytics workloads

### MongoDB (mongo/)
MongoDB database driver and components for document-based storage

### MySQL (mysql/)
MySQL database driver and components for relational data storage

### PostgreSQL (postgresql/)
PostgreSQL database driver and components for relational data storage

### Redis/Valkey (redis/)
Redis/Valkey database driver and components for in-memory data storage

### RocksDB (rocks/)
RocksDB key-value storage driver and components
- CMakeLists.txt - CMake build configuration for RocksDB component
- library.yaml - Library configuration file
- include/ - Public header files for RocksDB components
- src/ - Source code implementation for RocksDB components

### SQLite (sqlite/)
SQLite database driver and components for embedded relational storage

### YDB (ydb/)
YDB (Yandex Database) driver and components for distributed storage

## Communication Components

### gRPC (grpc/)
gRPC communication framework for high-performance RPC calls

### Kafka (kafka/)
Kafka messaging system integration for event streaming

### RabbitMQ (rabbitmq/)
RabbitMQ messaging system integration for message queuing

## Other Components

### ODBC (odbc/)
ODBC database connectivity layer

### OpenTelemetry (otlp/)
OpenTelemetry protocol implementation for distributed tracing and metrics

## Development Support

### CMake (cmake/)
CMake build system files and configurations for building userver applications

### Libraries (libraries/)
Additional libraries and dependencies

### Scripts (scripts/)
Development scripts and tools for various tasks

### Testsuite (testsuite/)
Testing framework and utilities for functional and integration testing

### Third Party (third_party/)
Third-party dependencies and libraries

### Universal (universal/)
Universal components that work across different systems

## Sample Services (samples/)
Example services demonstrating how to use various components of the userver framework

### Sample Services List
- chaotic_openapi_service/ - Service using OpenAPI code generation
- chaotic_service/ - Service using chaotic code generation
- clickhouse_service/ - Service using ClickHouse database
- config_service/ - Service demonstrating configuration management
- digest_auth_service/ - Service with digest authentication
- dns-resolver/ - Service demonstrating DNS resolution
- embedded_files/ - Service with embedded files
- flatbuf_service/ - Service using FlatBuffers serialization
- grpc_middleware_service/ - Service with gRPC middleware
- grpc_service/ - Service using gRPC communication
- grpc-generic-proxy/ - Generic gRPC proxy service
- hello_service/ - Basic hello world service example
- http_caching/ - Service demonstrating HTTP caching
- http_middleware_service/ - Service with HTTP middleware
- http-client-perf/ - HTTP client performance testing service
- https_service/ - Service with HTTPS support
- json2yaml/ - Service for converting JSON to YAML
- kafka_service/ - Service using Kafka messaging
- mongo_service/ - Service using MongoDB database
- mongo-support/ - MongoDB support utilities
- multipart_service/ - Service handling multipart requests
- mysql_service/ - Service using MySQL database
- netcat/ - Network utility service
- otlp_service/ - Service using OpenTelemetry protocol
- postgres_auth/ - Service with PostgreSQL authentication
- postgres_cache_order_by/ - Service with PostgreSQL caching
- postgres_service/ - Service using PostgreSQL database
- postgres-support/ - PostgreSQL support utilities
- production_service/ - Example production-ready service
- rabbitmq_service/ - Service using RabbitMQ messaging
- redis_service/ - Service using Redis database
- s3api/ - Service with S3 API integration
- static_service/ - Service serving static files
- tcp_full_duplex_service/ - Full-duplex TCP service
- tcp_service/ - TCP-based service
- testsuite-support/ - Testsuite support utilities
- websocket_service/ - Service with WebSocket support
- ydb_service/ - Service using YDB database

## Service Template (service_template/)
Template for creating new services with standard structure and configuration

### Service Template Components
- .clang-format - Code formatting configuration
- .gitignore - Git ignore patterns
- CMakeLists.txt - CMake build configuration
- CMakePresets.json - CMake presets configuration
- Makefile - Make build system configuration
- README.md - Service documentation
- requirements.txt - Python requirements
- run_as_user.sh - Script for running service as specific user
- .devcontainer/ - Development container configuration
- .github/ - GitHub workflow configurations
- cmake/ - CMake modules and configurations
- configs/ - Configuration files
- postgresql/ - PostgreSQL schema files
- proto/ - Protocol buffer definitions
- src/ - Source code implementation
- tests/ - Test files

## Memory Bank (memory-bank/)
Project-specific documentation and context files for AI agents

### Memory Bank Files
- projectbrief.md - Foundation document defining core requirements and goals
- productContext.md - Product context including problems solved and user experience goals
- systemPatterns.md - System architecture and technical decisions
- techContext.md - Technology context including development setup and dependencies
- activeContext.md - Current work focus, recent changes, and next steps
- progress.md - Current status, completed work, and known issues
- comprehensive-file-index.md - Detailed file index with descriptions (this document)