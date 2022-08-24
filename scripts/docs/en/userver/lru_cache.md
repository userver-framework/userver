# Least Recently Used (LRU) Caches and expirable LRU caches

A Least Recently Used (LRU) Cache organizes items in order of use. It drops
items not used for the longest amount of time in favor of caching new items.
There are also expirable LRU caches that drop expired records and may prolong 
record lifetime on usage.

To implement those caches userver provides a base component class 
cache::LruCacheComponent that manages an instance of cache::ExpirableLruCache.

## Typical use cases

LRU cache is useful for reducing the network interactions while working with
data that does not fit into service memory and rarely changes. Examples:
* caching user ids by their emails
* caching localized strings by their ids

Expirable LRU cache is useful for reducing the network interactions while
working with data does not fit into service memory and changes in predetermined
intervals. Examples:
* caching tokens that expire in an hour
* caching road average traffic situation for an area for a few minutes
* caching cab rating for a median travel time

## Usage

Using cache::LruCacheComponent is quite straightforward. Write a component that
derives from it:

@snippet cache/lru_cache_component_base_test.hpp  Sample lru cache component

After that, get the component using the 
components::ComponentContext::FindComponent() and call
cache::LruCacheComponent::GetCache(). Use the returned cache::LruCacheWrapper.

## Low level primitives

cache::LruCacheComponent should be your choice by default for implementing
all the LRU caches. However, in some cases there is a need to avoid using the
@ref md_en_userver_component_system "component system", or to avoid
synchronization, or to control the expiration logic more precisely.

Here's a brief introduction into LRU types:
* Concurrency-safe expirable component cache::LruCacheComponent. Call
  cache::LruCacheComponent::GetCache() to get a cache::LruCacheWrapper that
  provides actual interface to work with the cache.
* Concurrency-safe expirable cache::LruCacheWrapper is a smart pointer that
  provides simplified interface to the cache::ExpirableLruCache.
* Concurrency-safe expirable container cache::ExpirableLruCache with precise
  control over the expiration logic.
* Concurrency-safe non-expirable container cache::NWayLRU.
* Non-expirable container cache::LruMap that provides the same concurrency
  guarantees as the standard library containers.
* Non-expirable cache::LruSet that provides the same concurrency guarantees as
  the standard library containers.


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref pg_cache | @ref pg_driver ⇨
@htmlonly </div> @endhtmlonly
