# Caching Component for PostgreSQL

A typical components::PostgreCache usage consists of trait definition:

@snippet postgresql/src/cache/postgres_cache_test.cpp Pg Cache Policy Trivial

and registration of the component in components::ComponentList:

@snippet postgresql/src/cache/postgres_cache_test.cpp  Pg Cache Trivial Usage

See @ref scripts/docs/en/userver/caches.md for introduction into caches.


@section pg_cc_configuration Configuration

components::PostgreCache static configuration file should have a PostgreSQL
component name specified in `pgcomponent` configuration parameter.

Optionally the operation timeouts for cache loading can be specified.

For avoiding "memory leaks", see the respective section
in @ref components::CachingComponentBase.

@include{doc} scripts/docs/en/components_schema/postgresql/src/cache/base_postgres_cache.md

Options inherited from @ref components::CachingComponentBase :
@include{doc} scripts/docs/en/components_schema/core/src/cache/caching_component_base.md

Options inherited from @ref components::ComponentBase :
@include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md

@section pg_cc_cache_policy Cache policy

Cache policy is the template argument of components::PostgreCache component.
Please see the following code snippet for documentation.

@snippet cache/postgres_cache_test.cpp Pg Cache Policy Example

The query can be a std::string. But due to non-guaranteed order of static
data members initialization, std::string should be returned from a static
member function, please see the following code snippet.

@snippet cache/postgres_cache_test.cpp Pg Cache Policy GetQuery Example

Policy may have static function GetLastKnownUpdated. It should be used
when new entries from database are taken via revision, identifier, or
anything else, but not timestamp of the last update.
If this function is supplied, new entries are taken from db with condition
'WHERE kUpdatedField > GetLastKnownUpdated(cache_container)'.
Otherwise, condition is
'WHERE kUpdatedField > last_update - correction_'.
See the following code snippet for an example of usage

@snippet cache/postgres_cache_test.cpp Pg Cache Policy Custom Updated Example

Cache can also store only subset of data. For example for the database that is is defined in the following way:

@include samples/postgres_cache_order_by/schemas/postgresql/key_value.sql

it is possible to create a cache that stores only the latest `value`:

@snippet samples/postgres_cache_order_by/main.cpp  Last pg cache

In case one provides a custom CacheContainer within Policy, it is notified
of Update completion via its public member function OnWritesDone, if any.
Custom CacheContainer must provide size method and insert_or_assign method
similar to std::unordered_map's one or CacheInsertOrAssign function similar
to one defined in namespace utils::impl::projected_set (i.e. used for
utils::ProjectedUnorderedSet).
See the following code snippet for an example of usage:

@snippet cache/postgres_cache_test.cpp Pg Cache Policy Custom Container With Write Notification Example

@section pg_cc_forward_declaration Forward Declaration

To forward declare a cache you can forward declare a trait and
include userver/cache/base_postgres_cache_fwd.hpp header. It is also useful to
forward declare the cache value type.

@snippet cache/postgres_cache_test_fwd.hpp Pg Cache Fwd Example

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/cache_dumps.md | @ref scripts/docs/en/userver/lru_cache.md ⇨
@htmlonly </div> @endhtmlonly
