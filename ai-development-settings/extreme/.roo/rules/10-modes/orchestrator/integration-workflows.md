# Orchestrator Mode: Integration Workflow Rules

## Overview
This file defines orchestration patterns for managing complex integration workflows in userver environments, focusing on service integration, API coordination, database integration, and third-party service management.

## Service Integration Orchestration

### Inter-Service Communication Patterns
- **HTTP Client Integration**: Coordinate HTTP-based service communication
  - Plan [`clients::http::Client`](../../memory-bank/main/framework-core.md:1) configuration across services
  - Coordinate service discovery and load balancing
  - Design circuit breaker patterns for service resilience
  - Plan timeout and retry strategy coordination

- **gRPC Service Integration**: Coordinate gRPC-based communication
  - Plan gRPC service definition coordination
  - Coordinate protobuf schema evolution
  - Design gRPC middleware coordination
  - Plan gRPC client configuration management

### Service Mesh Integration
- **Service Mesh Coordination**: Orchestrate service mesh deployment
  - Plan service mesh configuration across services
  - Coordinate traffic management policies
  - Design security policy coordination
  - Plan observability integration

- **Load Balancing Coordination**: Coordinate load balancing strategies
  - Plan load balancer configuration
  - Coordinate health check integration
  - Design traffic distribution policies
  - Plan failover coordination

## API Integration Orchestration

### API Contract Management
- **API Versioning Coordination**: Manage API evolution
  - Plan API version compatibility matrices
  - Coordinate backward compatibility strategies
  - Design API deprecation workflows
  - Plan API migration coordination

- **OpenAPI Integration**: Coordinate API specification management
  - Plan OpenAPI schema coordination
  - Coordinate code generation workflows
  - Design API documentation integration
  - Plan API testing coordination

### API Gateway Integration
- **Gateway Coordination**: Orchestrate API gateway deployment
  - Plan API gateway configuration
  - Coordinate authentication and authorization
  - Design rate limiting coordination
  - Plan API monitoring integration

- **API Security Coordination**: Coordinate API security measures
  - Plan authentication strategy coordination
  - Coordinate authorization policy management
  - Design API key management
  - Plan security audit coordination

## Database Integration Orchestration

### Multi-Database Coordination
- **PostgreSQL Integration**: Coordinate PostgreSQL usage
  - Plan [`userver::postgresql`](../../memory-bank/main/framework-core.md:1) configuration
  - Coordinate connection pool management
  - Design transaction coordination
  - Plan database migration workflows

- **MongoDB Integration**: Coordinate MongoDB usage
  - Plan [`userver::mongo`](../../memory-bank/main/framework-core.md:1) configuration
  - Coordinate collection design strategies
  - Design document schema evolution
  - Plan data consistency strategies

- **Redis Integration**: Coordinate Redis/Valkey usage
  - Plan [`userver::redis`](../../memory-bank/main/framework-core.md:1) configuration
  - Coordinate caching strategies
  - Design data expiration policies
  - Plan Redis cluster coordination

### Database Migration Coordination
- **Schema Evolution Orchestration**: Coordinate database schema changes
  - Plan forward migration coordination
  - Coordinate rollback migration strategies
  - Design data transformation workflows
  - Plan migration validation procedures

- **Data Consistency Coordination**: Manage data consistency across databases
  - Plan eventual consistency strategies
  - Coordinate distributed transaction management
  - Design data synchronization workflows
  - Plan conflict resolution procedures

## Message Queue Integration Orchestration

### Kafka Integration Coordination
- **Kafka Service Integration**: Coordinate Kafka usage
  - Plan [`userver::kafka`](../../memory-bank/main/framework-core.md:1) configuration
  - Coordinate topic design and management
  - Design consumer group coordination
  - Plan message serialization strategies

- **Event-Driven Architecture**: Coordinate event-driven patterns
  - Plan event schema design and evolution
  - Coordinate event sourcing strategies
  - Design event replay mechanisms
  - Plan event ordering coordination

### RabbitMQ Integration Coordination
- **RabbitMQ Service Integration**: Coordinate RabbitMQ usage
  - Plan [`userver::rabbitmq`](../../memory-bank/main/framework-core.md:1) configuration
  - Coordinate queue design and management
  - Design message routing strategies
  - Plan dead letter queue handling

## Third-Party Service Integration

### External API Integration
- **Third-Party API Coordination**: Manage external service integration
  - Plan external API client configuration
  - Coordinate API rate limiting strategies
  - Design external service circuit breakers
  - Plan API key and authentication management

- **Webhook Integration**: Coordinate webhook handling
  - Plan webhook endpoint design
  - Coordinate webhook security validation
  - Design webhook retry mechanisms
  - Plan webhook monitoring and logging

### Cloud Service Integration
- **Cloud Provider Coordination**: Coordinate cloud service usage
  - Plan cloud service authentication
  - Coordinate cloud resource management
  - Design cloud service monitoring
  - Plan cloud cost optimization

- **S3 Integration Coordination**: Coordinate S3 API usage
  - Plan [`S3 API`](../../memory-bank/specialized/s3-integration/s3-patterns.md:1) integration
  - Coordinate bucket management strategies
  - Design object lifecycle policies
  - Plan S3 security coordination

## Configuration Integration Orchestration

### Dynamic Configuration Coordination
- **Dynamic Config Integration**: Coordinate [`dynamic_config`](../../memory-bank/main/framework-core.md:1) usage
  - Plan configuration schema coordination
  - Coordinate configuration update workflows
  - Design configuration validation strategies
  - Plan configuration rollback procedures

- **Environment Configuration**: Coordinate environment-specific configuration
  - Plan environment variable coordination
  - Coordinate secret management integration
  - Design configuration template management
  - Plan configuration deployment workflows

### Service Discovery Integration
- **Service Registry Coordination**: Coordinate service discovery
  - Plan service registration workflows
  - Coordinate service health check integration
  - Design service metadata management
  - Plan service deregistration procedures

## Monitoring Integration Orchestration

### Observability Integration
- **Distributed Tracing Integration**: Coordinate tracing across integrations
  - Plan trace context propagation across service boundaries
  - Coordinate span creation and correlation
  - Design trace sampling strategies
  - Plan trace data aggregation

- **Metrics Integration Coordination**: Coordinate metrics collection
  - Plan [`utils::statistics::Writer`](../../memory-bank/main/framework-core.md:1) integration
  - Coordinate custom metrics standardization
  - Design metrics aggregation strategies
  - Plan metrics alerting coordination

### Logging Integration Coordination
- **Centralized Logging**: Coordinate log aggregation
  - Plan structured logging coordination
  - Coordinate log correlation strategies
  - Design log retention policies
  - Plan log analysis workflows

## Testing Integration Orchestration

### Integration Testing Coordination
- **End-to-End Testing**: Coordinate comprehensive testing
  - Plan [`userver_testsuite_add()`](../../memory-bank/main/service-patterns.md:1) integration
  - Coordinate test environment provisioning
  - Design test data management
  - Plan test execution coordination

- **Contract Testing Coordination**: Coordinate API contract testing
  - Plan contract definition and validation
  - Coordinate contract evolution testing
  - Design contract compatibility testing
  - Plan contract regression testing

### Mock Service Coordination
- **Mock Service Management**: Coordinate test mocking
  - Plan mock service provisioning
  - Coordinate mock data management
  - Design mock service behavior
  - Plan mock service lifecycle management

## Security Integration Orchestration

### Authentication Integration
- **Authentication Coordination**: Coordinate authentication across services
  - Plan authentication provider integration
  - Coordinate token management strategies
  - Design authentication flow coordination
  - Plan authentication monitoring

- **Authorization Integration**: Coordinate authorization policies
  - Plan role-based access control coordination
  - Coordinate permission management
  - Design authorization policy enforcement
  - Plan authorization audit workflows

### Security Monitoring Integration
- **Security Event Coordination**: Coordinate security monitoring
  - Plan security event collection
  - Coordinate threat detection workflows
  - Design incident response coordination
  - Plan security audit integration

## Performance Integration Orchestration

### Performance Monitoring Coordination
- **Performance Metrics Integration**: Coordinate performance monitoring
  - Plan performance baseline establishment
  - Coordinate performance regression detection
  - Design performance optimization workflows
  - Plan capacity planning coordination

- **Load Testing Integration**: Coordinate load testing
  - Plan load testing environment coordination
  - Coordinate load testing data management
  - Design load testing scenario coordination
  - Plan load testing result analysis

## Cross-References

### Memory Bank Integration
- **Framework Core**: [`framework-core.md`](../../memory-bank/main/framework-core.md:1)
- **Component System**: [`component-system.md`](../../memory-bank/main/component-system.md:1)
- **Service Patterns**: [`service-patterns.md`](../../memory-bank/main/service-patterns.md:1)
- **Async Programming**: [`async-programming.md`](../../memory-bank/main/async-programming.md:1)

### Specialized Domains
- **S3 Integration**: [`s3-patterns.md`](../../memory-bank/specialized/s3-integration/s3-patterns.md:1)
- **TCP/UDP Networking**: [`networking-patterns.md`](../../memory-bank/specialized/tcp-udp-networking/networking-patterns.md:1)
- **WebSocket Advanced**: [`websocket-patterns.md`](../../memory-bank/specialized/websocket-advanced/websocket-patterns.md:1)
- **Advanced Monitoring**: [`monitoring-patterns.md`](../../memory-bank/specialized/advanced-monitoring/monitoring-patterns.md:1)

### Related Mode Rules
- **Workflow Coordination**: [`workflow-coordination.md`](./workflow-coordination.md:1)
- **Complex Projects**: [`complex-projects.md`](./complex-projects.md:1)
- **Team Collaboration**: [`team-collaboration.md`](./team-collaboration.md:1)
- **Release Orchestration**: [`release-orchestration.md`](./release-orchestration.md:1)

## Integration Workflow Checklists

### Service Integration Checklist
- [ ] Service communication patterns defined
- [ ] API contracts established and validated
- [ ] Service discovery configured
- [ ] Load balancing strategies implemented
- [ ] Circuit breaker patterns configured
- [ ] Health check integration operational
- [ ] Monitoring and observability integrated
- [ ] Security policies implemented

### Database Integration Checklist
- [ ] Database connection strategies defined
- [ ] Schema evolution procedures established
- [ ] Data consistency strategies implemented
- [ ] Migration workflows validated
- [ ] Backup and recovery procedures tested
- [ ] Performance optimization implemented
- [ ] Security measures configured
- [ ] Monitoring integration operational

### API Integration Checklist
- [ ] API versioning strategies defined
- [ ] API documentation generated and maintained
- [ ] API testing procedures implemented
- [ ] API security measures configured
- [ ] API monitoring and alerting operational
- [ ] API rate limiting configured
- [ ] API gateway integration completed
- [ ] API contract testing implemented

### Third-Party Integration Checklist
- [ ] External service dependencies identified
- [ ] Authentication and authorization configured
- [ ] Rate limiting and circuit breakers implemented
- [ ] Error handling and retry logic configured
- [ ] Monitoring and alerting operational
- [ ] Security measures implemented
- [ ] Documentation and runbooks created
- [ ] Testing procedures established

### Configuration Integration Checklist
- [ ] Configuration management strategy defined
- [ ] Environment-specific configurations validated
- [ ] Secret management implemented
- [ ] Configuration deployment workflows operational
- [ ] Configuration validation procedures implemented
- [ ] Configuration rollback procedures tested
- [ ] Configuration monitoring operational
- [ ] Configuration documentation maintained