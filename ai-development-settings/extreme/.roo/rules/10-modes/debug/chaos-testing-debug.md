# Debug Mode: Chaos Testing and Fault Injection

## Overview

This document provides comprehensive guidelines for using userver's chaos testing framework during debugging sessions. Chaos testing helps validate system resilience by simulating various network failures and fault conditions that can occur in production environments.

## Core Chaos Testing Components

### TcpGate - TCP Protocol Testing

The [`pytest_userver.chaos.TcpGate`](https://userver.tech/d4/d3d/classpytest__userver_1_1chaos_1_1TcpGate.html) component provides network proxy functionality for TCP-based protocols (HTTP, database connections, etc.).

```python
from pytest_userver import chaos

# Basic TcpGate setup
gate_config = chaos.GateRoute(
    name='postgres proxy',
    host_to_server=pgsql_local['key_value'].host,
    port_to_server=pgsql_local['key_value'].port,
)

async with chaos.TcpGate(gate_config) as proxy:
    # Test with proxy
    pass
```

### UdpGate - UDP Protocol Testing

The [`pytest_userver.chaos.UdpGate`](https://userver.tech/d8/de5/classpytest__userver_1_1chaos_1_1UdpGate.html) component handles UDP-based protocols (DNS, custom UDP services).

```python
# UDP gate for DNS testing
udp_gate_config = chaos.GateRoute(
    name='dns proxy',
    host_to_server='8.8.8.8',
    port_to_server=53,
)

async with chaos.UdpGate(udp_gate_config) as udp_proxy:
    # Test UDP-based services
    pass
```

## Network Failure Simulation Matrix

### Supported Network Conditions

| Network Condition | Production Causes | Debug Use Case |
|-------------------|-------------------|----------------|
| Server doesn't accept new connections | Server overload, network misconfiguration | Test connection pool exhaustion handling |
| Client/server doesn't read data from socket | Overload, deadlock, OS buffer overflow | Validate timeout and retry mechanisms |
| Client/server reads but doesn't respond | Overload, deadlock in threads | Test request timeout handling |
| Client/server closes connections | Restart, overload, connection limits | Validate reconnection logic |
| Socket closes when receiving data | Congestion control activation | Test backpressure handling |
| Data corruption | Memory corruption, stack overflow | Validate data integrity checks |
| Network delays | Poor network, interference | Test timeout configurations |
| Limited bandwidth | Network congestion | Test performance under constraints |
| Connection timeouts | Provider timeouts | Validate timeout propagation |
| Data slicing | Network interference, router issues | Test partial data handling |

## Chaos Testing Patterns

### 1. Database Connection Chaos

```python
@pytest.fixture(scope='session')
async def postgres_chaos_gate(pgsql_local):
    """Setup chaos gate for PostgreSQL testing"""
    gate_config = chaos.GateRoute(
        name='postgres proxy',
        host_to_server=pgsql_local['key_value'].host,
        port_to_server=pgsql_local['key_value'].port,
    )
    
    async with chaos.TcpGate(gate_config) as proxy:
        yield proxy

@pytest.fixture
async def gate(service_client, postgres_chaos_gate):
    """Reset gate to default state between tests"""
    await postgres_chaos_gate.to_server_pass()
    await postgres_chaos_gate.to_client_pass()
    postgres_chaos_gate.start_accepting()
    
    await postgres_chaos_gate.wait_for_connections()
    yield postgres_chaos_gate

async def test_database_connection_failure(service_client, gate):
    """Test database connection failure handling"""
    # Simulate connection rejection
    gate.stop_accepting()
    
    response = await service_client.get('/api/users')
    assert response.status == 500
    
    # Verify service recovers
    await _verify_service_recovery(service_client, gate)
```

### 2. HTTP Client Chaos Testing

```python
@pytest.fixture(scope='session')
async def http_service_chaos_gate():
    """Setup chaos gate for external HTTP service"""
    gate_config = chaos.GateRoute(
        name='external api proxy',
        host_to_server='api.external-service.com',
        port_to_server=443,
    )
    
    async with chaos.TcpGate(gate_config) as proxy:
        yield proxy

async def test_http_client_timeout(service_client, http_service_chaos_gate):
    """Test HTTP client timeout handling"""
    # Simulate slow responses
    await http_service_chaos_gate.to_client_delay(delay_ms=5000)
    
    response = await service_client.get('/api/external-data')
    assert response.status == 504  # Gateway timeout
    
    # Restore normal operation
    await http_service_chaos_gate.to_client_pass()
```

### 3. Data Corruption Testing

```python
async def test_data_corruption_handling(service_client, gate):
    """Test handling of corrupted network data"""
    # Corrupt data from server to client
    await gate.to_client_corrupt_data()
    
    response = await service_client.get('/api/data')
    # Service should detect corruption and return error
    assert response.status in [500, 502, 503]
    
    # Verify recovery
    await _verify_service_recovery(service_client, gate)

async def test_partial_data_corruption(service_client, gate):
    """Test handling of partially corrupted responses"""
    # Corrupt only client-to-server data
    await gate.to_server_corrupt_data()
    
    response = await service_client.post('/api/data', json={'key': 'value'})
    assert response.status in [400, 500]  # Bad request or server error
    
    await _verify_service_recovery(service_client, gate)
```

### 4. Connection Pool Exhaustion

```python
async def test_connection_pool_exhaustion(service_client, gate):
    """Test behavior when connection pool is exhausted"""
    # Close connections when data is received
    await gate.to_server_close_on_data()
    await gate.to_client_close_on_data()
    
    # Make multiple concurrent requests to exhaust pool
    tasks = []
    for _ in range(gate.connections_count() * 2):
        task = asyncio.create_task(
            service_client.get('/api/database-query')
        )
        tasks.append(task)
    
    responses = await asyncio.gather(*tasks, return_exceptions=True)
    
    # Some requests should fail due to connection issues
    error_count = sum(1 for r in responses if isinstance(r, Exception) or 
                     (hasattr(r, 'status') and r.status >= 500))
    assert error_count > 0
    
    await _verify_service_recovery(service_client, gate)
```

## Advanced Chaos Testing Scenarios

### 1. Cascading Failure Simulation

```python
async def test_cascading_failure(service_client, db_gate, cache_gate, external_api_gate):
    """Test system behavior under multiple simultaneous failures"""
    # Simulate database slowdown
    await db_gate.to_server_delay(delay_ms=2000)
    
    # Simulate cache unavailability
    cache_gate.stop_accepting()
    
    # Simulate external API errors
    await external_api_gate.to_client_close_on_data()
    
    # System should gracefully degrade
    response = await service_client.get('/api/complex-operation')
    assert response.status in [200, 503]  # Success with degraded functionality or service unavailable
    
    # Verify individual component recovery
    await _verify_multi_component_recovery(service_client, [db_gate, cache_gate, external_api_gate])
```

### 2. Network Partition Simulation

```python
async def test_network_partition(service_client, gate):
    """Test behavior during network partitions"""
    # Simulate complete network partition
    gate.stop_accepting()
    await gate.to_server_close_on_data()
    await gate.to_client_close_on_data()
    
    # Service should detect partition and respond appropriately
    response = await service_client.get('/health')
    assert response.status in [503, 500]  # Service unavailable
    
    # Simulate partition healing
    await _heal_network_partition(gate)
    
    # Verify service recovery
    await _verify_service_recovery(service_client, gate)
```

### 3. Intermittent Failure Testing

```python
async def test_intermittent_failures(service_client, gate):
    """Test handling of intermittent network issues"""
    success_count = 0
    failure_count = 0
    
    for i in range(20):
        if i % 3 == 0:  # Every third request fails
            await gate.to_server_close_on_data()
        else:
            await gate.to_server_pass()
        
        try:
            response = await service_client.get('/api/data')
            if response.status == 200:
                success_count += 1
            else:
                failure_count += 1
        except Exception:
            failure_count += 1
    
    # Service should handle intermittent failures gracefully
    assert success_count > 0  # Some requests should succeed
    assert failure_count > 0  # Some requests should fail
    
    # Ensure final recovery
    await gate.to_server_pass()
    await _verify_service_recovery(service_client, gate)
```

## Chaos Testing Utilities

### Service Recovery Verification

```python
import asyncio
import logging

logger = logging.getLogger(__name__)

async def _verify_service_recovery(service_client, gate):
    """Verify that service recovers after chaos injection"""
    # Restore normal gate operation
    await gate.to_server_pass()
    await gate.to_client_pass()
    gate.start_accepting()
    
    # Consume dead connections
    await _consume_dead_connections(service_client)
    
    # Verify service is healthy
    response = await service_client.get('/health')
    assert response.status == 200, "Service failed to recover after chaos injection"

async def _consume_dead_connections(service_client, max_pool_size=10):
    """Force consumption of dead database connections"""
    logger.debug("Starting dead connection consumption")
    
    # Make requests to consume dead connections
    tasks = [
        service_client.get('/api/simple-query') 
        for _ in range(max_pool_size * 2)
    ]
    
    await asyncio.gather(*tasks, return_exceptions=True)
    
    # Verify connections are working
    response = await service_client.get('/api/simple-query')
    assert response.status == 200
    
    logger.debug("Dead connection consumption completed")

async def _heal_network_partition(gate):
    """Restore network connectivity after partition"""
    await gate.to_server_pass()
    await gate.to_client_pass()
    gate.start_accepting()
    await gate.wait_for_connections()
```

### Chaos Testing Configuration

```python
# conftest.py - Chaos testing setup
@pytest.fixture(name='userver_config_testsuite', scope='session')
def _userver_config_testsuite(userver_config_testsuite):
    """Configure userver for chaos testing"""
    def patch_config(config_yaml, config_vars):
        userver_config_testsuite(config_yaml, config_vars)
        
        components = config_yaml['components_manager']['components']
        testsuite_support = components['testsuite-support']
        
        # Disable timeouts for chaos testing
        testsuite_support['testsuite-pg-execute-timeout'] = '0ms'
        testsuite_support['testsuite-pg-statement-timeout'] = '0ms'
        testsuite_support.pop('testsuite-pg-readonly-master-expected', None)
    
    return patch_config

@pytest.fixture(scope='session')
def userver_pg_config(pgsql_local, postgres_chaos_gate):
    """Configure PostgreSQL to use chaos gate"""
    def _hook_db_config(config_yaml, config_vars):
        host, port = postgres_chaos_gate.get_sockname_for_clients()
        db_info = pgsql_local['key_value']
        
        db_chaos_gate = connection.PgConnectionInfo(
            host=host,
            port=port,
            user=db_info.user,
            password=db_info.password,
            options=db_info.options,
            sslmode=db_info.sslmode,
            dbname=db_info.dbname,
        )
        
        components = config_yaml['components_manager']['components']
        db = components['key-value-database']
        db['dbconnection'] = db_chaos_gate.get_uri()
    
    return _hook_db_config
```

## Debug-Specific Chaos Testing

### 1. Memory Leak Detection Under Stress

```python
async def test_memory_leaks_under_chaos(service_client, gate):
    """Test for memory leaks during network failures"""
    initial_memory = await _get_memory_usage(service_client)
    
    # Run chaos scenarios multiple times
    for _ in range(10):
        await gate.to_server_close_on_data()
        
        # Generate load
        tasks = [service_client.get('/api/data') for _ in range(50)]
        await asyncio.gather(*tasks, return_exceptions=True)
        
        await gate.to_server_pass()
        await _verify_service_recovery(service_client, gate)
    
    final_memory = await _get_memory_usage(service_client)
    
    # Memory usage shouldn't grow significantly
    memory_growth = final_memory - initial_memory
    assert memory_growth < initial_memory * 0.1  # Less than 10% growth
```

### 2. Deadlock Detection

```python
async def test_deadlock_detection_under_chaos(service_client, gate):
    """Test for deadlocks during network failures"""
    # Simulate conditions that might cause deadlocks
    await gate.to_server_delay(delay_ms=1000)
    
    # Generate concurrent load
    tasks = []
    for _ in range(20):
        tasks.append(service_client.post('/api/complex-transaction', json={'data': 'test'}))
    
    # All requests should complete (no deadlocks)
    responses = await asyncio.wait_for(
        asyncio.gather(*tasks, return_exceptions=True),
        timeout=30.0  # Reasonable timeout
    )
    
    # Verify no requests are stuck
    assert len(responses) == 20
    
    await _verify_service_recovery(service_client, gate)
```

### 3. Resource Cleanup Verification

```python
async def test_resource_cleanup_after_chaos(service_client, gate):
    """Verify proper resource cleanup after network failures"""
    # Get initial resource counts
    initial_stats = await _get_resource_stats(service_client)
    
    # Run chaos scenario
    await gate.to_client_corrupt_data()
    
    # Generate requests that will fail
    for _ in range(100):
        try:
            await service_client.get('/api/resource-intensive')
        except Exception:
            pass  # Expected failures
    
    await gate.to_client_pass()
    await _verify_service_recovery(service_client, gate)
    
    # Verify resources are cleaned up
    final_stats = await _get_resource_stats(service_client)
    
    assert final_stats['open_files'] <= initial_stats['open_files'] + 5
    assert final_stats['active_connections'] <= initial_stats['active_connections'] + 2
```

## Integration with Monitoring

### Chaos Testing Metrics

```python
async def test_chaos_with_metrics_validation(service_client, gate):
    """Validate metrics during chaos testing"""
    # Get baseline metrics
    baseline_metrics = await _get_service_metrics(service_client)
    
    # Run chaos scenario
    await gate.to_server_close_on_data()
    
    # Generate load
    for _ in range(50):
        try:
            await service_client.get('/api/data')
        except Exception:
            pass
    
    # Check metrics reflect the chaos
    chaos_metrics = await _get_service_metrics(service_client)
    
    # Error rates should increase
    assert chaos_metrics['error_rate'] > baseline_metrics['error_rate']
    
    # Connection errors should be recorded
    assert chaos_metrics['connection_errors'] > baseline_metrics['connection_errors']
    
    await _verify_service_recovery(service_client, gate)
```

## Best Practices

### 1. Test Design Principles

- **Gradual Escalation**: Start with simple failures, escalate to complex scenarios
- **Recovery Validation**: Always verify service recovery after chaos injection
- **Realistic Scenarios**: Model real-world failure patterns
- **Isolation**: Test individual failure modes before combining them

### 2. Chaos Testing Hygiene

- Reset gates between tests to ensure clean state
- Use appropriate timeouts for chaos scenarios
- Monitor resource usage during chaos tests
- Document expected behaviors for each failure mode

### 3. Production Readiness Validation

- Test all critical paths under various failure conditions
- Validate circuit breaker and retry mechanisms
- Ensure graceful degradation under partial failures
- Verify monitoring and alerting work during failures

### 4. Debugging Integration

- Correlate chaos test results with application logs
- Use metrics to understand system behavior during failures
- Implement chaos testing in CI/CD pipelines
- Create chaos testing playbooks for production incidents

This chaos testing framework provides comprehensive fault injection capabilities for validating userver application resilience and debugging failure scenarios in controlled environments.