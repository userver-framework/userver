#pragma once

#include <memory>  // for std::shared_ptr

#include <userver/cache/mysql/cache.hpp>

USERVER_NAMESPACE_BEGIN

namespace example {  // replace with a namespace of your trait

struct MySqlExamplePolicy;
struct MyStructure;

}  // namespace example

namespace caches {

using MyCache1 = components::MySqlCache<example::MySqlExamplePolicy>;
using MyCache1Data = std::shared_ptr<const example::MyStructure>;

}  // namespace caches
/*! [Pg Cache Fwd Example] */

USERVER_NAMESPACE_END