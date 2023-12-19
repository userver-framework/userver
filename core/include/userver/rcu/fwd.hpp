#pragma once

/// @file userver/rcu/fwd.hpp
/// @brief Forward declarations for rcu::Variable and rcu::RcuMap

#include <functional>
#include <unordered_map>

USERVER_NAMESPACE_BEGIN

namespace engine {

class Mutex;

}  // namespace engine

namespace rcu {

template <typename T>
struct DefaultRcuTraits;

template <typename Key, typename Value>
struct DefaultRcuMapTraits;

template <typename T, typename RcuTraits = DefaultRcuTraits<T>>
class Variable;

template <typename T, typename RcuTraits = DefaultRcuTraits<T>>
class ReadablePtr;

template <typename T, typename RcuTraits = DefaultRcuTraits<T>>
class WritablePtr;

template <typename Key, typename Value,
          typename RcuMapTraits = DefaultRcuMapTraits<Key, Value>>
class RcuMap;

}  // namespace rcu

USERVER_NAMESPACE_END
