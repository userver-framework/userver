# Load Testing Patterns

## Overview
Comprehensive guide for load testing userver applications to ensure they meet performance requirements under various load conditions.

## Load Testing Framework

### TestSuite Integration
Userver provides functional testing capabilities through TestSuite, which can be extended for load testing:

```python
# Example load test using pytest and TestSuite
import pytest

async def test_high_concurrency_load(service_client):
    """Test service under high concurrency load"""
    # Configure test parameters
    concurrent_requests = 1000
    total_requests = 10000
    
    # Perform load test
    responses = await service_client.run_concurrent_requests(
        '/api/endpoint',
        method='GET',
        count=total_requests,
        concurrency=concurrent_requests
    )
    
    # Validate results
    assert all(r.status == 200 for r in responses)
    assert avg_response_time < 100  # ms
```

### Load Testing Tools

#### JMeter Integration
Configure JMeter for userver load testing:
```xml
<!-- JMeter test plan example -->
<TestPlan>
    <ThreadGroup>
        <stringProp name="ThreadGroup.num_threads">100</stringProp>
        <stringProp name="ThreadGroup.ramp_time">30</stringProp>
        <stringProp name="ThreadGroup.duration">300</stringProp>
    </ThreadGroup>
    <HTTPSamplerProxy>
        <stringProp name="HTTPSampler.domain">localhost</stringProp>
        <stringProp name="HTTPSampler.port">8080</stringProp>
        <stringProp name="HTTPSampler.path">/api/endpoint</stringProp>
        <stringProp name="HTTPSampler.method">GET</stringProp>
    </HTTPSamplerProxy>
</TestPlan>
```

#### k6 Integration
Use k6 for modern load testing:
```javascript
import http from 'k6/http';
import { check, sleep } from 'k6';

export let options = {
  stages: [
    { duration: '30s', target: 100 },  // ramp up
    { duration: '1m', target: 100 },   // steady state
    { duration: '30s', target: 0 },    // ramp down
  ],
};

export default function () {
  const res = http.get('http://localhost:8080/api/endpoint');
  check(res, {
    'status is 200': (r) => r.status === 200,
    'response time < 200ms': (r) => r.timings.duration < 200,
  });
  sleep(1);
}
```

## Load Testing Scenarios

### Baseline Performance Testing
Establish baseline performance metrics:
- Response time percentiles (50th, 95th, 99th)
- Throughput (requests per second)
- Error rates
- Resource utilization (CPU, memory, network)

### Stress Testing
Push the system beyond normal operating conditions:
- Gradually increase load until failure
- Identify breaking points
- Measure recovery time
- Document performance degradation patterns

### Soak Testing
Run extended load tests to identify:
- Memory leaks
- Resource exhaustion
- Performance degradation over time
- Stability under sustained load

### Spike Testing
Test response to sudden load increases:
- Simulate traffic spikes
- Measure system responsiveness
- Validate auto-scaling behavior
- Check error handling during overload

## Load Testing Metrics

### Key Performance Indicators
- **Response Time**: Average, median, 95th percentile, 99th percentile
- **Throughput**: Requests per second, transactions per second
- **Error Rate**: Percentage of failed requests
- **Availability**: Uptime percentage during test
- **Resource Utilization**: CPU, memory, disk I/O, network I/O

### Service-Level Objectives (SLOs)
Define measurable performance targets:
```yaml
# Example SLO configuration
slos:
  - name: "API Response Time"
    metric: "http_response_time_95th_percentile"
    target: 100  # ms
    threshold: 150  # ms
  - name: "API Availability"
    metric: "http_success_rate"
    target: 99.9  # percentage
    threshold: 99.5  # percentage
```

## Database Load Testing

### Connection Pool Testing
Test database connection pool behavior:
- Maximum concurrent connections
- Connection acquisition time
- Connection reuse efficiency
- Pool exhaustion scenarios

### Query Performance Testing
Validate database query performance:
- Query execution time under load
- Index effectiveness
- Lock contention
- Transaction isolation impact

## Load Testing Best Practices

### Test Environment Setup
- Use production-like hardware specifications
- Configure identical network conditions
- Populate with realistic data volumes
- Match production configuration settings

### Test Data Management
- Generate realistic test data
- Ensure data privacy and compliance
- Maintain data consistency across tests
- Use data masking for sensitive information

### Monitoring During Tests
- Monitor system resources in real-time
- Track application metrics
- Capture logs for analysis
- Use distributed tracing for request flow

### Result Analysis
- Compare results against baseline
- Identify performance bottlenecks
- Document findings and recommendations
- Create performance regression tests

## Load Testing Anti-Patterns

### Common Mistakes
- Testing in development environment only
- Using unrealistic test data
- Ignoring warm-up periods
- Not monitoring system resources
- Testing single endpoints in isolation
- Ignoring network latency effects

### What to Avoid
- Don't test with empty databases
- Don't ignore cache warming
- Don't run tests during production hours
- Don't skip baseline measurements
- Don't ignore error analysis
- Don't test without monitoring

## Load Testing Tools Integration

### Prometheus Metrics Collection
Collect and analyze metrics during load tests:
```bash
# Example Prometheus query for load testing
rate(http_requests_total[5m])  # Requests per second
histogram_quantile(0.95, http_request_duration_seconds_bucket)  # 95th percentile
```

### Grafana Dashboards
Create dashboards for load test monitoring:
- Real-time performance metrics
- Resource utilization graphs
- Error rate tracking
- Custom business metrics

### Automated Load Testing
Integrate load testing into CI/CD pipeline:
```yaml
# Example CI/CD integration
load_test:
  stage: test
  script:
    - k6 run load-test.js
    - python analyze_results.py
    - if [ "$ERROR_RATE" -gt 1 ]; then exit 1; fi
```

## Load Testing Checklist

### Pre-Test Preparation
- [ ] Define test objectives and success criteria
- [ ] Set up test environment matching production
- [ ] Prepare realistic test data
- [ ] Configure monitoring and logging
- [ ] Establish baseline performance metrics

### Test Execution
- [ ] Execute warm-up period
- [ ] Monitor system resources
- [ ] Capture performance metrics
- [ ] Document any anomalies
- [ ] Run multiple test iterations

### Post-Test Analysis
- [ ] Analyze performance data
- [ ] Identify bottlenecks and issues
- [ ] Compare against baseline metrics
- [ ] Document findings and recommendations
- [ ] Create performance regression tests