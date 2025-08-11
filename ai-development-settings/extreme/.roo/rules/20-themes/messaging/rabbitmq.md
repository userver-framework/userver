# RabbitMQ Patterns and Message Queue Management

## Overview

RabbitMQ integration in userver provides access to RabbitMQ servers via `components::RabbitMQ`. The uRabbitMQ driver is asynchronous and suspends the current coroutine for carrying out network I/O operations.

**Quality Tier**: Golden Tier

## Core Features

### Message Operations
- Publishing messages to exchanges
- Consuming messages from queues
- Creating Exchanges, Queues and Bindings
- Transport level security (TLS)
- Connections pooling for efficient resource utilization
- End-to-end logging for messages in publish->consume chain

### Component Configuration

```yaml
rabbitmq-component-name:
  secdist_alias: rabbitmq-alias
  min_pool_size: 5
  max_pool_size: 10
  max_in_flight_requests: 5
  use_secure_connection: true
```

### Key Configuration Options
- `secdist_alias`: Name of the key in secdist config (defaults to component name)
- `min_pool_size`: Minimum connections pool size (per host, defaults to 5)
- `max_pool_size`: Maximum connections pool size (per host, consumers excluded, defaults to 10)
- `max_in_flight_requests`: Per-connection limit for requests awaiting response (defaults to 5)
- `use_secure_connection`: Whether to use TLS for connections (defaults to true)

## Implementation Patterns

### Publishing Messages
1. **Get Client Instance**: Access the RabbitMQ client through `components::RabbitMQ::GetClient()`
2. **Create Channel**: Use `urabbitmq::Channel` or `urabbitmq::ReliableChannel` for message operations
3. **Publish Messages**: Send messages to exchanges with appropriate routing keys
4. **Handle Publishing Errors**: Implement proper error handling for network issues and broker errors

### Consuming Messages
1. **Consumer Base Classes**: Use `urabbitmq::ConsumerBase` and `urabbitmq::ConsumerComponentBase`
2. **Message Processing**: Implement efficient message processing callbacks
3. **Acknowledgment Patterns**: Properly acknowledge or reject messages based on processing results
4. **Error Handling**: Handle consumer errors and implement restart strategies

### Administration Operations
1. **Admin Channel**: Use `urabbitmq::AdminChannel` for administrative operations
2. **Exchange Management**: Create and configure exchanges with appropriate types (direct, topic, fanout, headers)
3. **Queue Management**: Create and configure queues with appropriate durability and TTL settings
4. **Binding Management**: Create bindings between exchanges and queues with routing patterns

## Best Practices

### Connection Management
1. **Pool Configuration**: Configure appropriate pool sizes based on expected load
2. **Connection Lifecycle**: Properly handle connection establishment and recovery
3. **Resource Cleanup**: Ensure connections are properly closed and cleaned up
4. **Monitoring**: Monitor connection pool metrics and connection errors

### Message Design
1. **Message Structure**: Design clear and consistent message formats
2. **Routing Keys**: Use meaningful routing keys for effective message routing
3. **Message Properties**: Set appropriate message properties (delivery mode, headers, etc.)
4. **Content Encoding**: Use appropriate content encoding and serialization formats

### Performance Optimization
1. **Batch Operations**: Use batch publishing when possible for better throughput
2. **Prefetch Count**: Configure appropriate prefetch counts for consumers
3. **Connection Reuse**: Leverage connection pooling to minimize connection overhead
4. **Asynchronous Operations**: Use asynchronous patterns to maximize concurrency

## Security Considerations

### Authentication and Authorization
1. **User Management**: Proper user and permission management in RabbitMQ
2. **Virtual Hosts**: Use virtual hosts for tenant isolation
3. **Access Control**: Implement proper access control policies
4. **Credential Management**: Secure credential storage and rotation

### Transport Security
1. **TLS Configuration**: Enable and properly configure TLS connections
2. **Certificate Management**: Proper certificate validation and management
3. **Network Security**: Secure network communication between services and RabbitMQ
4. **Audit Logging**: Enable and monitor security-related audit logs

## Error Handling and Recovery

### Common Error Scenarios
1. **Connection Failures**: Handle broker connection failures gracefully
2. **Message Rejection**: Handle message rejection and dead lettering
3. **Channel Errors**: Handle channel-level errors and recovery
4. **Broker Unavailability**: Implement proper retry and fallback strategies

### Recovery Strategies
1. **Automatic Reconnection**: Leverage built-in reconnection mechanisms
2. **Message Redelivery**: Handle message redelivery scenarios
3. **Circuit Breaker**: Implement circuit breaker patterns for external dependencies
4. **Graceful Degradation**: Provide fallback mechanisms for non-critical operations

## Cross-References

- [components::RabbitMQ](../../../../../d5/d23/classcomponents_1_1RabbitMQ.html)
- [urabbitmq::Client](../../../../../da/d66/classurabbitmq_1_1Client.html)
- [urabbitmq::AdminChannel](../../../../../d4/de8/classurabbitmq_1_1AdminChannel.html)
- [urabbitmq::Channel](../../../../../da/d8b/classurabbitmq_1_1Channel.html)
- [urabbitmq::ReliableChannel](../../../../../db/deb/classurabbitmq_1_1ReliableChannel.html)
- [urabbitmq::ConsumerBase](../../../../../d2/d52/classurabbitmq_1_1ConsumerBase.html)
- [urabbitmq::ConsumerComponentBase](../../../../../d8/d65/classurabbitmq_1_1ConsumerComponentBase.html)