# MultiIndex LRU Container

## Introduction

Generic LRU (Least Recently Used) cache container implementation that combines Boost.MultiIndex for flexible indexing with efficient LRU tracking. The container maintains elements in access order while supporting multiple indexing strategies through Boost.MultiIndex. The LRU eviction policy automatically removes the least recently accessed items when capacity is reached.

Two container variants are provided:
- **Container** - Basic LRU cache with capacity management
- **ExpirableContainer** - Extended version with time-based expiration (TTL)


## API Reference

### Container Class

#### Constructors
- `explicit Container(size_t max_size)` - Creates container with specified capacity

#### Modifiers
- `emplace(Args&&... args)` - Constructs element in-place, returns pair<iterator, bool>
- `insert(const Value& value)` - Inserts copy of value
- `insert(Value&& value)` - Inserts moved value
- `erase<Tag, Key>(const Key& key)` - Erases element by key, returns bool
- `clear()` - Removes all elements
- `set_capacity(size_t new_capacity)` - Adjusts container capacity

#### Lookup
- `find<Tag, Key>(const Key& key)` - Finds element, updates LRU order
- `find_no_update<Tag, Key>(const Key& key) const` - Finds without updating LRU
- `contains<Tag, Key>(const Key& key)` - Checks existence, updates LRU
- `contains_no_update<Tag, Key>(const Key& key) const` - Checks without updating
- `equal_range<Tag, Key>(const Key& key)` - Returns range of matching elements, updates all
- `equal_range_no_update<Tag, Key>(const Key& key) const` - Returns range without updates

#### Capacity
- `size() const` - Returns number of elements
- `empty() const` - Checks if container is empty
- `capacity() const` - Returns maximum capacity

#### Iterators
- `end<Tag>()` - Returns end iterator for specified index

### ExpirableContainer Class

#### Constructors
- `ExpirableContainer(size_t max_size, std::chrono::milliseconds ttl)` - Creates with capacity and TTL

#### Additional Methods
- `cleanup_expired()` - Removes all expired items from container

All other methods have the same interface as Container, with the following behavior differences:
- **find()** - Returns expired items as end iterator; updates timestamp on access
- **equal_range()** - Filters out expired items; updates timestamps for valid items
- **contains()** - Returns false for expired items; updates timestamp for valid items

## Implementation Notes

### Node Reuse Strategy
The container maintains a free list of allocated nodes to reduce memory allocations. When items are evicted or erased, their nodes are moved to the free list and reused for future insertions.

### Thread Safety
This container is **not thread-safe**. External synchronization is required for concurrent access.

### Iterator Invalidation
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
