# LRU Container

## Introduction

Generic LRU (Least Recently Used) cache container implementation that combines Boost.MultiIndex for flexible indexing with Boost.Intrusive for efficient LRU tracking. It uses `namespace multi_index_lru`.

The container maintains elements in access order while supporting multiple indexing strategies through Boost.MultiIndex. The LRU eviction policy automatically removes the least recently accessed items when capacity is reached.

## Usage

```cpp
#include <userver/utils/multi_index_lru.hpp>

using MyLruCache = multi_index_lru::LRUCacheContainer<
    MyValue,
    boost::multi_index::indexed_by<
        boost::multi_index::hashed_unique<
            boost::multi_index::tag<MyTag>,
            boost::multi_index::member<MyValue, std::string, &MyValue::key>
        >
    >
>;

MyLruCache cache(1000); // Capacity of 1000 items
cache.insert(my_value);
auto it = cache.find<MyTag>("some_key");
```

