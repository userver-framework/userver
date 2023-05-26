# Basics of Caches

A cache in userver is a @ref md_en_userver_component_system "component" that
periodically polls an external resource and caches its response. Strictly
speaking, the cache component is a shadow replica of some resource
(database tables, mongo collections, etc.). The use of the cache is
that the user receives the current data snapshot and works with
that snapshot without waiting for updates. Cache updates occur asynchronously
in the background and do not block users from working with the cache.

As a result, the user usually works with slightly outdated data, but at the same
time always has an instant access to some version of the data.

Caches usually inherit from components::CachingComponentBase or
cache::LruCacheComponent. Sections below describe the features of
components::CachingComponentBase. For information on cache::LruCacheComponent
refer to @ref md_en_userver_lru_cache.


## Update Modes

Caches have two update modes:
* Full update. In this mode, the cache requests its full state from an external
  resource.
* Incremental update. In this mode, the cache requests changes since the last
  Full update, patching its current state.

If there has not been a successful
full update for a long time (more than the value of `full-update-interval`),
then the next update will be full. In particular, if there is a full update
error, the next update will also be full.

The cache can disable incremental updates and work only in full updates
mode, or vice versa - work only in incremental updates mode. The very first
update is always full, regardless of the settings.

Here's a sample update mode settings of components::CachingComponentBase
based cache:
```
yaml
  mongo-dynamic-config-cache:
    update-interval: 5s
    update-jitter: 1s
    full-update-interval: 10s
    update-types: full-and-incremental
```

To disable incremental/full updates, specify this in the `update-types` tag, for example:
```
yaml
  js-pipeline-compilation:
    # Can be only-full, only-incremental, full-and-incremental
    update-types: only-full
    update-interval: 180s
```

### Update intervals

`update-interval` is the time interval between the `Update` calls.
In particular this means:
- `Update` is not called concurrently
- Only with `update-jitter: 0s` and `is-strong-period: true` updates follow a
  strict schedule.

`full-update-interval` is the minimum interval between `full` updates in
`full-and-incremental` cache. With each update, the cache checks if 
`full-update-interval` of the time has passed since the previous `full` update,
and if it has passed, the current update will be `full`.

`update-jitter` describes the spread in the delay between cache updates. It is
especially useful in the case of heavy caches, which significantly load the
database/remote with their updates. Adding a random delay spreads the cache
updates of various instances over time, thereby removing the peak load on the
database/remote.


## Fault Tolerance

The user can get the current cache snapshot regardless of the current state of
the database/remote from which the cache is being poured, provided that at
least once the cache update worked correctly. The user is working with a local
copy, which may be slightly behind the original in time. In case of problems
with the database/remote the cache update may finish with an error (i.e. the
`Update` method will throw an exception), leaving the local cache unchanged.

By default, the first cache update must be completed successfully to set the
initial value. If the first update fails with an error, i.e. the update throws
an exception, then this leads to an exception in the cache component
constructor and to the service shutdown. There are two ways to change
this behavior:
1. @ref md_en_userver_cache_dumps "By enabling dumps", in this case the cache
    starts with the state stored in the dump. If there is no dump, the service
    will still fall when the first update fails. You can overcome this
    through next clause
2. By giving a permission to load without the first update. In case of
    auxiliary cache, it may be useful to leave the cache in an error state
    and asynchronously update it after the service start.
    To do this, specify `first-update-fail-ok` in the component settings:
```
yaml
  first-update-fail-ok-cache:
  update-interval: 60s
  update-jitter: 10s
  full-update-interval: 1000s
  first-update-fail-ok: true
```

If the "cache has no data" situation is normal for you and you want to handle
it yourself, you can override the `MayReturnNull()` method in the cache so that
it returns `true` (by default `false`). In this case, instead of an exception,
you will get an empty smart pointer (`nullptr`).


## Access synchronization, versioning, memory consumption

Cache implementation does not block
multiple concurrent cache readers while doing a background
cache update. In case of time consuming work with cache data, users may encounter
the fact that different readers work with different versions of the data. As a
result, more than two versions of the cache data can coexist at the same time.
Memory and other resources of old data versions are released either when 
updating the cache or when the background garbage collector collects the
garbage. The frequency of garbage collection is regulated via the
`additional-cleanup-interval` parameter.

The total amount of memory for the cache can be approximately calculated by the formula:
```
Total cache memory = cache data memory * ([user time / update interval] + 1)
```
where:
- `cache data memory` is the maximum size of one cache copy
- `update interval` - minimum cache update period
- `user time` - the maximum time that users hold a copy of the cache (i.e. the
  lifetime of the `Get()` result)

For example, if the cache data occupies 1 GB, the cache is updated every 4
seconds, and the handle works for 9 seconds, then the total memory consumption
of the cache is `1 GB * ([9/4]+1) = 4 GB`.

A commonly used technique to solve the problem of excessive memory consumption
for large caches is splitting the cache into chunks.

## Heavy Caches

Updating caches can significantly load the CPU, for example, when parsing data
from a database. If such a load lasts for a significant time without context
switches (several tens of milliseconds), then this may negatively affect the
timings of the service. Sometimes a similar problem occurs not only when
forming a new cache snapshot, but also in the callback of a cache subscription,
for example, when forming an additional cache index. There are several ways to
solve this problem.

**The first option**. Increase the number of CPU cores. The more cores you have
in the service, the less CPU fraction is used by caches and the better is
the efficiency of the service. On the contrary, when splitting a cluster into small
containers with a small number of cores, the number of cache instances in the
cluster increases, which is why the consumed by caches CPU fraction increases.

**The second option**. Add context switching. If you call
engine::Yield() every 1-2ms, then the framework engine has time to execute the
other ready for execution coroutines and the queue of ready coroutines does not
grow to undesirable values. To simplify working with engine::Yield, it is
recommended to use utils::CpuRelax rather than calling engine::Yield() manually.

## Specializations for DB

Caches over DB are caching components that use a trait structure as a
template argument for customization. Such components are:

- components::MongoCache
- components::PostgreCache

A typical case of cache usage consists of trait structure definition:

@snippet cache/postgres_cache_test.cpp Pg Cache Policy Trivial

and registration of the caching component in components::ComponentList:

@snippet cache/postgres_cache_test.cpp  Pg Cache Trivial Usage

The DB caches are simple to use and quite poverfull.

## Writing a cache

To write your own cache using only the components::CachingComponentBase base
class, you need to override the cache::CacheUpdateTrait::Update,
implement Full and Incremental updates. In Update(), don't forget to put down
metrics for the `stats_scope` object, describing how many objects were read,
how many parsing errors there were, and how many elements are in the final cache.

See @ref md_en_userver_tutorial_http_caching for a detailed introduction.


## Parallel loading

Cache components, like other components, are loaded in parallel. This allows
you to speed up the loading of the service in the case of multiple heavy caches.


## Metrics

Each cache automatically collects metrics. See
@ref md_en_userver_service_monitor for a list of metrics with some descriptions. 


## Common misuses

* Non-const (mutable) data in cache without synchronization:
  ```
  struct CacheData {
    int integer;                        // fine
    std::shared_ptr<std::string> name;  // bad, `const CacheData` allows mutating content of `*name`
  };
  ```
  Users of the cache could harm themselves by concurrently mutating thread-unsafe
  types. A fixed varsion of the above structure would be:
  ```
  struct CacheData {
    int integer;
    std::shared_ptr<const std::string> name;  // now const!
  };
  ```
* Storing a large number of independent std::unordered_maps in the cache at
  once. If you need to get 3 different groups of data from 3 different sources,
  make 3 independent caches. This allows them to update independently,
  use standard cache metrics, etc.


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref md_en_userver_os_signals | @ref md_en_userver_cache_dumps ⇨
@htmlonly </div> @endhtmlonly
