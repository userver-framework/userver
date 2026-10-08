## Valkey/Redis

**Quality:** @ref QUALITY_TIERS "Platinum Tier".

The redis asynchronous driver provides an interface to work with Valkey or Redis
standalone instances, as well as Valkey/Redis Sentinels and Clusters.

Take note that Valkey/Redis is not able to guarantee a strong consistency. It is
possible for Valkey/Redis to lose writes that were acknowledged, and vice versa.

## Main features

* Convenient methods for Valkey/Redis commands returning proper C++ types
* Support for bulk operations (MGET, MSET, etc). Driver splits data into smaller
  chunks if necessary to increase server responsiveness
* Support for different strategies of choosing the most suitable server instance
* Request timeouts management with transparent retries
* TLS connections support
* @ref scripts/docs/en/userver/deadline_propagation.md
* Cluster autotopology


## Valkey/Redis Guarantees

Valkey/Redis is not a reliable database by design. In case of problems on the database
side, partial or complete loss of data is possible. When a master migrates, the
entire cluster may become unavailable for several tens of seconds. The
specific latency value should be determined for each configuration
separately (depending on the size of the database, server location, etc.).

Every command that is sent to the server has the potential to fail. Moreover,
the client may receive error information (for example, a timeout), but the
command on the server may succeed. Therefore, Valkey/Redis commands should be
idempotent. If this is not possible for some reason, then care should be taken
to ensure that incomplete groups of commands/resent commands do not leave the
database in an inconsistent state.

A Valkey/Redis command has a timeout, a number of retries, and a global timeout. If a
response is not received from the server within the timeout, then the same
command is sent to another server in the cluster, and so on either until the
limit on the number of repetitions is reached, or when the global timeout is
reached. These settings can be changed via storages::redis::CommandControl.

@warning For the above reasons, it is recommended to prefer PostgreSQL database
         over Valkey/Redis. However it is fine to use Valkey/Redis as a distributed cache.

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

See @ref scripts/docs/en/userver/service_monitor.md for info on how to get the metrics.

## Usage

To use Valkey or Redis you must add the component components::Redis and configure it
according to the documentation. After that you can make requests via
storages::redis::Client:

@snippet redis/src/storages/redis/client_redistest.cpp Sample Redis Client usage

Also see @ref scripts/docs/en/userver/tutorial/redis_service.md for a complete example.


### Optimistic transactions

Valkey 9.2 and newer support optimistic transactions with conditional `EXEC`.
Pass @ref storages::redis::ExecOptions to @ref storages::redis::Transaction::Exec:

@snippet redis/src/storages/redis/optimistic_transaction_test.cpp optimistic transaction sample

See the [Valkey EXEC documentation](https://valkey.io/commands/exec/) for the supported conditions
and the [Valkey transaction documentation](https://valkey.io/topics/transactions/) for their semantics.

If a condition fails, `Exec().Get()` and the subcommand requests throw
@ref storages::redis::TransactionAbortedException. Read the current values again before
constructing a new transaction to retry. Server and transport errors retain their usual exception types.

Conditional transactions run on the master. Keys in commands and conditions
must belong to the same shard, and to the same hash slot in cluster mode.
An empty @ref storages::redis::ExecOptions is equivalent to ordinary `Exec` and works with older
servers. The conditions are consumed during `Exec`, so an initializer list can be passed directly.

To mock conditional transactions, override @ref storages::redis::MockTransactionImplBase::Exec:
return @ref storages::redis::MockTransactionImplBase::ExecResult::kExecuted to execute the mocked
subrequests, or @ref storages::redis::MockTransactionImplBase::ExecResult::kAborted to report a
transaction abort. The default mock rejects non-empty conditions so that they cannot be silently ignored.

### Timeouts

Request timeout can be set for a single request via storages::redis::CommandControl
argument.

Dynamic option @ref REDIS_DEFAULT_COMMAND_CONTROL can be used to set default
values.

To interrupt a request on the client side, you can use
@ref task_cancellation_intro "cancellation mechanism" to cancel the task
that executes the Redis request:

@snippet redis/src/storages/redis/client_redistest.cpp Sample Redis Cancel request

Valkey/Redis driver does not guarantee that the cancelled request was not executed by the server.


### Valkey/Redis Cluster Autotopology

Cluster autotopology makes it possible to do resharding of the cluster
without Secdist changes and service restart.

With this feature entries in the Secdist are now treated not as a list of all
the cluster hosts, but only as input points through which the service discowers
the configuration of the entire cluster. Therefore, it is not recommended
to delete instances that are listed in secdist from the cluster.

The cluster configuration is checked
* at the start of the service
* and periodically
* and if a MOVED response is received from Valkey/Redis

If a change in the cluster topology was detected during the check
(hashslot distribution change, master change, or new replicas discowered),
then the internal representation of the topology is recreated,
the new topology is gets ready (new connections may appear in it),
and after that the active topology is replaced.

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/mongodb.md | @ref scripts/docs/en/userver/clickhouse/driver.md ⇨
@htmlonly </div> @endhtmlonly
