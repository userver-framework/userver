# MultiIndex LRU Container

Generic LRU (Least Recently Used) cache container implementation that combines Boost.MultiIndex for flexible indexing
with efficient LRU tracking. The container maintains elements in access order while supporting multiple indexing
strategies through Boost.MultiIndex. The LRU eviction policy automatically removes the least recently accessed items
when capacity is reached.

Two container variants are provided:
- **Container** - Basic LRU cache with capacity management
- **ExpirableContainer** - Extended version with time-based expiration (TTL)

## MultiIndex LRU Implementation Notes

### MultiIndex LRU Node Reuse Strategy
The container maintains a free list of allocated nodes to reduce memory allocations. When items are evicted or erased,
their nodes are moved to the free list and reused for future insertions.

### MultiIndex LRU Thread Safety
This container is **not thread-safe**. External synchronization is required for concurrent access.

### MultiIndex LRU Iterator Invalidation
- Insertions may invalidate iterators if capacity is exceeded and eviction occurs
- Erasures invalidate iterators to the erased element only
- Find operations do not invalidate iterators
- `set_capacity()` may invalidate iterators when reducing capacity


## Usage

@snippet libraries/multi-index-lru/src/container_test.cpp Usage


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/libraries/grpc-reflection.md | @ref scripts/docs/en/userver/development/stability.md ⇨
@htmlonly </div> @endhtmlonly
