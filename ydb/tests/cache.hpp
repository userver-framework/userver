#pragma once

#include <memory>  // for std::shared_ptr

#include <userver/cache/ydb/cache.hpp>

USERVER_NAMESPACE_BEGIN

namespace example {  // replace with a namespace of your trait

struct YdbExamplePolicy;
struct YdbStructure;

}  // namespace example

namespace caches {

using MyCache1 = components::YdbCache<example::YdbExamplePolicy>;
using MyCache1Data = std::shared_ptr<const example::YdbStructure>;

}  // namespace caches


USERVER_NAMESPACE_END