# uPg Driver

**Quality:** @ref QUALITY_TIERS "Platinum Tier".

🐙 **userver** provides access to PostgreSQL database servers via
components::Postgres. The uPg driver is asynchronous, it suspends
current coroutine for carrying out network I/O.

## Postgres driver Features
- PostgreSQL cluster topology discovery;
- Manual cluster sharding (access to shard clusters by index);
- Connection pooling;
- Queries are transparently converted to prepared statements to use less
  network on next execution, give the database more optimization freedom,
  avoid the need for parameters escaping as the latter are now send separately from the query;
- Query construction on the fly via @ref storages::postgres::ParameterStore.
- Automatic PgaaS topology discovery;
- Selecting query target (master/slave);
- Connection failover;
- Transaction support via @ref storages::postgres::Transaction RAII wrapper;
- Variadic template query parameter passing;
- Query result extraction to C++ types;
- More effective binary protocol usage for communication rather than the libpq's default text protocol;
- Caching the low-level database (D)escribe responses to save about a half
  of network bandwidth on select statements that return multiple columns
  (compared to the libpq implementation);
- Portals for effective background cache updates;
- Queries pipelining to execute multiple queries in one network roundtrip
  (for example `begin + set transaction timeout + insert` result in one roundtrip);
- Ability to manually control network roundtrips via
  @ref storages::postgres::QueryQueue to gain maximum efficiency in case of multiple unrelated select statements;
- Mapping PostgreSQL user types to C++ types;
- Transaction error injection via pytest_userver.sql.RegisteredTrx;
- `LISTEN`/`NOTIFY` support via @ref storages::postgres::Cluster::Listen();
- @ref scripts/docs/en/userver/deadline_propagation.md .

## Transaction pooling with a PostgreSQL balancer

In session pooling mode, a PostgreSQL backend connection is assigned to a
client connection for the whole session, so session state is preserved between
transactions. In transaction pooling mode, the backend connection is returned
to the pool after each transaction, and the next transaction may use a
different connection. This allows more clients to share fewer backend
connections, but the service must not rely on session state being preserved
between transactions.

Transaction pooling with persistent prepared statements is supported only with
Odyssey's prepared statement reservation enabled:

```text
pool "transaction"
pool_reserve_prepared_statement yes
```

To switch a running service without restarting it, apply the changes in the
following order:

1. Set `pooler-mode` to `transaction` and
   `persistent-prepared-statements` to `false` in the
   @ref POSTGRES_CONNECTION_SETTINGS dynamic config:

   ```json
   {
     "postgres-component-name": {
       "pooler-mode": "transaction",
       "persistent-prepared-statements": false
     }
   }
   ```

2. Reload Odyssey with `pool "transaction"` and
   `pool_reserve_prepared_statement yes` for the corresponding route.
3. After the Odyssey reload has completed, set
   `persistent-prepared-statements` back to `true`:

   ```json
   {
     "postgres-component-name": {
       "pooler-mode": "transaction",
       "persistent-prepared-statements": true
     }
   }
   ```

Replace `postgres-component-name` with the PostgreSQL component's `name_alias`,
or with the component name if `name_alias` is not set.

@section toc More information
- For configuration see components::Postgres
- For cluster topology see storages::postgres::Cluster
- @ref scripts/docs/en/userver/pg/transactions.md
- @ref scripts/docs/en/userver/pg/run_queries.md
- @ref scripts/docs/en/userver/pg/process_results.md
- @ref scripts/docs/en/userver/pg/types.md
- @ref scripts/docs/en/userver/pg/user_types.md
- @ref scripts/docs/en/userver/pg/errors.md
- @ref scripts/docs/en/userver/pg/topology.md

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/lru_cache.md | @ref scripts/docs/en/userver/pg/transactions.md ⇨
@htmlonly </div> @endhtmlonly
