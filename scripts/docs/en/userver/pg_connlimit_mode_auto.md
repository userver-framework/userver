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

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref pg_topology | @ref scripts/docs/en/userver/pg_user_types.md ⇨
@htmlonly </div> @endhtmlonly
