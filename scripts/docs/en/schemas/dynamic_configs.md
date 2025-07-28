@anchor REDIS_WAIT_CONNECTED
## REDIS_WAIT_CONNECTED

Dynamic config that controls if services will wait for connections with redis
instances.

@include redis/dynamic_configs/REDIS_WAIT_CONNECTED.yaml

**Example:**
```json
{
  "mode": "master_or_slave",
  "throw_on_fail": false,
  "timeout-ms": 11000
}
```

Used by components::Redis.
