#pragma once

#include <functional>

#include <userver/cache/impl/hash.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache::impl::sketch {
enum Policy { Trivial = 0, Bloom, Doorkeeper, Aged };
template <typename T, typename Hash = std::hash<T>,
          Policy policy = Policy::Bloom>
class Sketch {};
}  // namespace cache::impl::sketch

USERVER_NAMESPACE_END
