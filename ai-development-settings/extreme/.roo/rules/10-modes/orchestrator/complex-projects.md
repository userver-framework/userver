# Orchestrator Mode: Complex Project Management Rules

## Overview
This file defines orchestration strategies for managing complex, multi-service userver projects with intricate dependencies, multiple development teams, and sophisticated deployment requirements.

## Complex Project Architecture Patterns

### Multi-Service System Design
- **Service Mesh Architecture**: Orchestrate interconnected userver services
  - Design service communication patterns using HTTP and gRPC
  - Plan service discovery and load balancing strategies
  - Coordinate shared component usage across services
  - Manage service versioning and compatibility matrices

- **Domain-Driven Service Boundaries**: Organize services by business domains
  - Identify bounded contexts for service separation
  - Design aggregate patterns within services
  - Plan event-driven communication between domains
  - Coordinate shared data models and schemas

### Dependency Management Strategies
- **Service Dependency Graphs**: Map and manage complex dependencies
  - Create dependency visualization and documentation
  - Identify circular dependencies and resolution strategies
  - Plan service startup and shutdown sequences
  - Coordinate database migration dependencies

- **Component Sharing Patterns**: Manage shared components across services
  - Design reusable [`components`](../../memory-bank/main/component-system.md:1) libraries
  - Coordinate component version management
  - Plan component interface evolution strategies
  - Manage component configuration across services

## Large-Scale Build Orchestration

### Multi-Repository Coordination
- **Monorepo vs Multi-repo Strategies**: Choose appropriate repository structure
  - Coordinate build systems across repositories
  - Manage shared library dependencies
  - Plan code sharing and reuse strategies
  - Coordinate version control workflows

- **Build Pipeline Orchestration**: Manage complex build workflows
  - Coordinate parallel build execution across services
  - Manage build artifact dependencies
  - Plan incremental build strategies
  - Coordinate build environment consistency

### Advanced CMake Coordination
- **Complex CMake Hierarchies**: Manage sophisticated build configurations
  - Coordinate [`CMakePresets.json`](../../memory-bank/main/framework-core.md:1) across projects
  - Manage feature flag propagation with [`USERVER_FEATURE_*`](../../memory-bank/main/component-system.md:1)
  - Coordinate third-party dependency resolution
  - Plan cross-compilation strategies

- **Build Optimization Strategies**: Optimize build performance
  - Coordinate distributed build systems
  - Plan build caching strategies
  - Optimize dependency resolution
  - Coordinate build parallelization

## Advanced Testing Orchestration

### Multi-Service Testing Strategies
- **Integration Testing Coordination**: Test service interactions
  - Coordinate test environment provisioning
  - Plan test data management across services
  - Design contract testing strategies
  - Coordinate end-to-end test execution

- **Performance Testing Orchestration**: Coordinate performance validation
  - Plan load testing across service boundaries
  - Coordinate performance baseline establishment
  - Design stress testing scenarios
  - Plan performance regression detection

### Advanced Testsuite Coordination
- **Complex Test Scenarios**: Orchestrate sophisticated test workflows
  - Coordinate [`userver_testsuite_add()`](../../memory-bank/main/service-patterns.md:1) across services
  - Plan test execution sequencing
  - Coordinate mock service management
  - Design test data isolation strategies

- **Chaos Testing Integration**: Coordinate chaos engineering practices
  - Plan failure injection across services
  - Coordinate resilience testing scenarios
  - Design recovery validation procedures
  - Plan chaos testing automation

## Database and Storage Orchestration

### Multi-Database Coordination
- **Database Strategy Planning**: Coordinate multiple database systems
  - Plan PostgreSQL, MongoDB, Redis coordination
  - Design data consistency strategies
  - Coordinate database migration workflows
  - Plan backup and recovery procedures

- **Data Migration Orchestration**: Manage complex data migrations
  - Coordinate schema evolution across services
  - Plan data transformation workflows
  - Design rollback procedures
  - Coordinate data validation strategies

### Storage Architecture Patterns
- **Distributed Storage Strategies**: Coordinate storage across services
  - Plan data partitioning strategies
  - Coordinate caching layers
  - Design data replication patterns
  - Plan storage scaling strategies

## Advanced Configuration Management

### Multi-Environment Coordination
- **Environment Strategy**: Coordinate configuration across environments
  - Plan development, staging, production coordination
  - Coordinate [`dynamic_config`](../../memory-bank/main/framework-core.md:1) management
  - Design configuration validation workflows
  - Plan secret management strategies

- **Configuration Evolution**: Manage configuration changes
  - Coordinate configuration versioning
  - Plan configuration migration strategies
  - Design configuration rollback procedures
  - Coordinate configuration testing

### Service Configuration Coordination
- **Cross-Service Configuration**: Manage shared configuration
  - Coordinate service discovery configuration
  - Plan load balancing configuration
  - Design circuit breaker coordination
  - Coordinate monitoring configuration

## Advanced Monitoring and Observability

### Distributed System Monitoring
- **Cross-Service Observability**: Coordinate monitoring across services
  - Plan distributed tracing strategies
  - Coordinate metrics aggregation
  - Design alerting strategies
  - Plan log correlation workflows

- **Performance Monitoring Coordination**: Monitor system performance
  - Coordinate performance metrics collection
  - Plan performance baseline management
  - Design performance regression detection
  - Coordinate capacity planning

### Advanced Metrics Orchestration
- **Business Metrics Coordination**: Coordinate business-level metrics
  - Plan KPI tracking across services
  - Coordinate business event tracking
  - Design business intelligence workflows
  - Plan metrics-driven decision making

## Deployment and Release Orchestration

### Complex Deployment Strategies
- **Multi-Service Deployment**: Coordinate service deployments
  - Plan blue-green deployment strategies
  - Coordinate canary deployment workflows
  - Design rollback procedures
  - Plan deployment validation

- **Infrastructure Orchestration**: Coordinate infrastructure management
  - Plan container orchestration strategies
  - Coordinate service mesh deployment
  - Design infrastructure scaling
  - Plan disaster recovery procedures

### Release Management Coordination
- **Release Planning**: Coordinate complex releases
  - Plan feature flag coordination
  - Coordinate release validation
  - Design release communication
  - Plan release rollback procedures

## Team Coordination Strategies

### Multi-Team Development
- **Team Boundary Management**: Coordinate team responsibilities
  - Plan service ownership models
  - Coordinate shared component ownership
  - Design team communication protocols
  - Plan knowledge sharing strategies

- **Development Workflow Coordination**: Coordinate development processes
  - Plan code review workflows
  - Coordinate integration procedures
  - Design conflict resolution processes
  - Plan development environment management

### Communication and Documentation
- **Documentation Orchestration**: Coordinate documentation efforts
  - Plan API documentation strategies
  - Coordinate architecture documentation
  - Design runbook management
  - Plan knowledge base maintenance

## Risk Management and Mitigation

### Technical Risk Orchestration
- **Risk Assessment**: Identify and manage technical risks
  - Plan dependency risk assessment
  - Coordinate security risk management
  - Design performance risk mitigation
  - Plan scalability risk assessment

- **Contingency Planning**: Prepare for complex scenarios
  - Plan disaster recovery procedures
  - Coordinate incident response workflows
  - Design business continuity plans
  - Plan technical debt management

## Cross-References

### Memory Bank Integration
- **Framework Core**: [`framework-core.md`](../../memory-bank/main/framework-core.md:1)
- **Component System**: [`component-system.md`](../../memory-bank/main/component-system.md:1)
- **Service Patterns**: [`service-patterns.md`](../../memory-bank/main/service-patterns.md:1)
- **Troubleshooting**: [`troubleshooting-guide.md`](../../memory-bank/main/troubleshooting-guide.md:1)

### Specialized Domains
- **Advanced Monitoring**: [`monitoring-patterns.md`](../../memory-bank/specialized/advanced-monitoring/monitoring-patterns.md:1)
- **Chaos Testing**: [`chaos-patterns.md`](../../memory-bank/specialized/chaos-testing/chaos-patterns.md:1)
- **Performance Research**: [`performance-research.md`](../../memory-bank/research/performance-research.md:1)
- **Future Directions**: [`future-directions.md`](../../memory-bank/research/future-directions.md:1)

### Related Mode Rules
- **Workflow Coordination**: [`workflow-coordination.md`](./workflow-coordination.md:1)
- **Architect Mode**: [`system-design.md`](../architect/system-design.md:1)
- **Debug Mode**: [`troubleshooting-workflows.md`](../debug/troubleshooting-workflows.md:1)

## Complex Project Checklists

### Project Architecture Checklist
- [ ] Service boundaries defined and validated
- [ ] Dependency graphs mapped and analyzed
- [ ] Communication patterns designed
- [ ] Data flow patterns established
- [ ] Security boundaries identified
- [ ] Performance requirements defined
- [ ] Scalability requirements planned

### Development Coordination Checklist
- [ ] Team responsibilities defined
- [ ] Development workflows established
- [ ] Code review processes implemented
- [ ] Integration procedures defined
- [ ] Testing strategies coordinated
- [ ] Documentation standards established
- [ ] Communication protocols defined

### Deployment Readiness Checklist
- [ ] Deployment strategies validated
- [ ] Infrastructure requirements met
- [ ] Monitoring systems operational
- [ ] Rollback procedures tested
- [ ] Performance baselines established
- [ ] Security validations completed
- [ ] Team training completed

### Risk Management Checklist
- [ ] Technical risks identified and assessed
- [ ] Mitigation strategies implemented
- [ ] Contingency plans developed
- [ ] Incident response procedures defined
- [ ] Business continuity plans validated
- [ ] Recovery procedures tested
- [ ] Risk monitoring systems operational