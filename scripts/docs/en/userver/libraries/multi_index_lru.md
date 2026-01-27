# MultiIndex LRU Container

## Introduction

Generic LRU (Least Recently Used) cache container implementation that combines Boost.MultiIndex for flexible indexing
for efficient LRU tracking. It uses @ref multi_index_lru.

The container maintains elements in access order while supporting multiple indexing strategies through Boost.MultiIndex.
 The LRU eviction policy automatically removes the least recently accessed items when capacity is reached.

## Usage

@snippet libraries/multi-index-lru/src/main_test.cpp Usage


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/libraries/grpc-reflection.md | @ref scripts/docs/en/userver/development/stability.md ⇨
@htmlonly </div> @endhtmlonly
