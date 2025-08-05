# Orchestrator Mode: Workflow Coordination Rules

## Overview
This file defines orchestration patterns for coordinating complex, multi-phase userver development workflows involving multiple teams, services, and deployment environments.

## Multi-Phase Project Coordination

### Project Breakdown Strategy
- **Service Decomposition**: Break complex projects into independent userver services
  - Identify service boundaries based on business domains
  - Plan service dependencies and communication patterns
  - Design service templates using [`userver-create-service`](../../memory-bank/main/service-patterns.md:1)
  - Coordinate shared component usage across services

### Task Orchestration Patterns
- **Parallel Development Streams**: Coordinate concurrent development tracks
  - Core service implementation
  - Database schema evolution
  - API contract development
  - Testing infrastructure setup
  - Documentation and deployment preparation

- **Dependency Resolution**: Manage inter-service dependencies
  - Service startup order coordination
  - Database migration sequencing
  - Configuration dependency mapping
  - Component initialization orchestration

### Integration Checkpoints
- **Phase Gates**: Define clear checkpoints for multi-phase projects
  - Service template validation
  - Component integration verification
  - Database connectivity confirmation
  - API contract compliance
  - Performance baseline establishment

## Build System Coordination

### CMake Orchestration
- **Multi-Service Builds**: Coordinate builds across service boundaries
  - Use [`CMakePresets.json`](../../memory-bank/main/framework-core.md:1) for consistent configuration
  - Manage [`USERVER_FEATURE_*`](../../memory-bank/main/component-system.md:1) flags across services
  - Coordinate dependency resolution with CPM
  - Orchestrate parallel build execution

- **Environment Coordination**: Manage build environments
  - Dev Container orchestration for team consistency
  - Docker image coordination for deployment
  - Build artifact management and distribution
  - Cross-platform build coordination

### Dependency Management
- **Library Coordination**: Manage userver library dependencies
  - Coordinate [`find_package(userver COMPONENTS ...)`](../../memory-bank/main/framework-core.md:1) usage
  - Manage version compatibility across services
  - Orchestrate third-party dependency updates
  - Handle conflicting dependency requirements

## Testing Workflow Orchestration

### Test Suite Coordination
- **Multi-Level Testing**: Orchestrate different test types
  - Unit test execution coordination
  - Functional test orchestration with [`userver_testsuite_add()`](../../memory-bank/main/service-patterns.md:1)
  - Integration test sequencing
  - Chaos testing coordination
  - Performance test orchestration

- **Test Environment Management**: Coordinate test environments
  - Database setup and teardown sequencing
  - Mock service coordination
  - Test data management across services
  - Parallel test execution coordination

### Continuous Integration Orchestration
- **CI Pipeline Coordination**: Manage complex CI workflows
  - Multi-service build orchestration
  - Test execution sequencing
  - Artifact promotion workflows
  - Quality gate enforcement
  - Deployment pipeline coordination

## Service Lifecycle Orchestration

### Startup Coordination
- **Service Dependencies**: Orchestrate service startup order
  - Database readiness verification
  - External service dependency checks
  - Configuration validation sequencing
  - Health check coordination
  - Component initialization ordering

- **Configuration Management**: Coordinate configuration across services
  - [`dynamic_config`](../../memory-bank/main/framework-core.md:1) synchronization
  - Environment-specific configuration deployment
  - Secret management coordination
  - Configuration validation workflows

### Runtime Coordination
- **Service Communication**: Orchestrate inter-service communication
  - HTTP client configuration coordination
  - gRPC service discovery management
  - Message queue coordination
  - Circuit breaker coordination
  - Load balancing orchestration

## Monitoring and Observability Coordination

### Metrics Orchestration
- **Cross-Service Metrics**: Coordinate metrics collection
  - [`utils::statistics::Writer`](../../memory-bank/main/framework-core.md:1) coordination
  - Prometheus metrics aggregation
  - Custom metrics standardization
  - Performance metrics correlation
  - Business metrics coordination

- **Distributed Tracing**: Orchestrate tracing across services
  - Trace context propagation
  - Span correlation across service boundaries
  - Performance bottleneck identification
  - Request flow visualization
  - Error propagation tracking

### Logging Coordination
- **Centralized Logging**: Orchestrate log aggregation
  - Log format standardization
  - Correlation ID propagation
  - Log level coordination
  - Structured logging enforcement
  - Log retention policy coordination

## Deployment Orchestration

### Release Coordination
- **Multi-Service Releases**: Coordinate releases across services
  - Version compatibility verification
  - Database migration coordination
  - Configuration update sequencing
  - Rollback procedure coordination
  - Canary deployment orchestration

- **Environment Promotion**: Orchestrate environment promotions
  - Development to staging promotion
  - Staging to production deployment
  - Configuration environment coordination
  - Data migration orchestration
  - Rollback capability verification

## Cross-References

### Memory Bank Integration
- **Core Framework**: [`framework-core.md`](../../memory-bank/main/framework-core.md:1)
- **Component System**: [`component-system.md`](../../memory-bank/main/component-system.md:1)
- **Service Patterns**: [`service-patterns.md`](../../memory-bank/main/service-patterns.md:1)
- **Async Programming**: [`async-programming.md`](../../memory-bank/main/async-programming.md:1)

### Specialized Domains
- **Chaos Testing**: [`chaos-patterns.md`](../../memory-bank/specialized/chaos-testing/chaos-patterns.md:1)
- **Advanced Monitoring**: [`monitoring-patterns.md`](../../memory-bank/specialized/advanced-monitoring/monitoring-patterns.md:1)
- **Performance Research**: [`performance-research.md`](../../memory-bank/research/performance-research.md:1)

### Related Mode Rules
- **Architect Mode**: [`system-design.md`](../architect/system-design.md:1)
- **Debug Mode**: [`troubleshooting-workflows.md`](../debug/troubleshooting-workflows.md:1)
- **Code Mode**: [`implementation-patterns.md`](../code/implementation-patterns.md:1)

## Workflow Checklists

### Project Initiation Checklist
- [ ] Service architecture defined and validated
- [ ] Service templates created and configured
- [ ] Build system coordination established
- [ ] Testing strategy defined and implemented
- [ ] Deployment pipeline designed
- [ ] Monitoring and observability planned
- [ ] Team coordination protocols established

### Integration Checkpoint Checklist
- [ ] Service interfaces validated
- [ ] Database schemas synchronized
- [ ] Configuration management verified
- [ ] Testing infrastructure operational
- [ ] Monitoring systems configured
- [ ] Documentation updated
- [ ] Deployment procedures tested

### Release Coordination Checklist
- [ ] Version compatibility verified
- [ ] Database migrations tested
- [ ] Configuration updates validated
- [ ] Rollback procedures verified
- [ ] Monitoring alerts configured
- [ ] Performance baselines established
- [ ] Team communication completed