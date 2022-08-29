## Redis

The redis asynchronous driver provides an interface to work with Redis
standalone instances, as well as Redis Sentinels and Clusters.

Take note that Redis is not able to guarantee a strong consistency. It is
possible for Redis to lose writes that were acknowledged, and vice
versa.

## Main features

* Convenient methods for Redis commands returning proper C++ types;
* Support for bulk operations (MGET, MSET, etc); driver splits data into smaller
  chunks if necessary to increase server responsiveness;
* Support for different strategies of choosing the most suitable Redis instance;
* Request timeouts management with transparent retries.

## Metrics

| Metric name           | Description                              |
|-----------------------|------------------------------------------|
| redis.instances_count | current number of Redis instances        |
| redis.last_ping_ms    | last measured ping value                 |
| redis.is_ready        | 1 if connected and ready, 0 otherwise    |
| redis.not_ready_ms    | milliseconds since last ready status     |
| redis.reconnects      | reconnect counter                        |
| redis.session-time-ms | milliseconds since connected to instance |
| redis.timings         | query timings                            |
| redis.errors          | counter of failed requests               |

See @ref md_en_userver_service_monitor for info on how to get the metrics.

## Usage

To use Redis you must add the component components::Redis and configure it
according to the documentation. After that you can make requests via 
storages::redis::Client:

@snippet storages/redis/client_redistest.cpp Sample Redis Client usage

### Timeouts

Request timeout can be set for a single request via redis::CommandControl 
argument.

Dynamic option @ref REDIS_DEFAULT_COMMAND_CONTROL can be used to set default 
values.

To interrupt a request on the client side, you can use 
@ref task_cancellation_intro "cancellation mechanism" to cancel the task 
that executes the Redis request:

@snippet storages/redis/client_redistest.cpp Sample Redis Cancel request

Redis driver does not guarantee that the cancelled request was not executed
by the server.

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref md_en_userver_mongodb | @ref clickhouse_driver ⇨
@htmlonly </div> @endhtmlonly
