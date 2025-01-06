# uPg: Automatic connection limit

## Server connection limit

A PostgreSQL server has a connection limit for all clients.
If there are too many client connections, it may lead to random connection closing.
You have to thoroughly calculate client instance count in the cluster and
serverside connection limit to get maximum client pool size.
Moreover, you have to recalculate the limit in case of cluster resize.

It is a whole thing to maintain the limit all the time and it is error-prone.
A wrong setup might lead to incidents.

That's why we have "connlimit-mode: auto" that override the components::Postgres
static config option `max_pool_size`.

## connlimit-mode: auto

The PostgreSQL driver allows one to automatically determine the required max
connection limit. It is calculated as follows:

```
client_max_connections = server_max_connections/instances - reserved
```

`server_max_connections` is calculated based on `SHOW max_connections` answer and
on the current user connection limit.
The service identifies `instanses` based on the following algorithm.
The service creates `u_clients` table and regularly writes down information about
itself. After that the service reads the table and identifies alive instances.
`reserved` is set to be 5 to reflect administrative scripts connections,
migration scripts, etc.

The limit is promptly changed after service topology change: node
addition/removal, service instance stop, etc.

## Disabling the feature

The feature can be turned on/off in runtime via the dynamic config variable
@ref POSTGRES_CONNLIMIT_MODE_AUTO_ENABLED. If disabled dynamically, the previous
connection limit settings are used.

If you want to disable the feature permanently for some clusters (e.g. in case
the current user may not create `u_clients` table), you can do it in static config
for components::Postgres:

```
components_manager:
    components:
        postgres-xxx:
            connlimit_mode: manual
```

## Error "No available connections found"

This error tells that there are no available connections in the pool. But it doesn't 
mean that the service exceeded the limit of the number of connections.

The pg-driver will try to create a new connection asynchronously (with the deadline from the initial request).
If another connection is released from another request, driver will take it. 
If request spikes are expected, you might increase the `min_pool_size` option (default 4) in components::Postgres config to prepare
some available connections for these request spikes.
Also, if there are logs such as `Connecting: X. Max concurrent connecting: X.`, you might increase the `connecting_limit` option (default unlimited)
in components::Postgres config. Also, you can change these options by the dynamic config @ref POSTGRES_CONNECTION_POOL_SETTINGS

@warning No new connections will be created if there are many errors when creating a connection in the recent period. 
The recent error period is 15 seconds. Error count is controlled by the option `recent-errors-threshold` (default 2) in the dynamic config @ref POSTGRES_CONNECTION_SETTINGS. 
Errors accounted for the purposes of `recent-errors-threshold` are: 
1) A timeout error when creating a connection 
2) A ConnectionError (aborted connections and pipeline mode errors)

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref pg_topology | @ref scripts/docs/en/userver/pg_user_types.md ⇨
@htmlonly </div> @endhtmlonly
