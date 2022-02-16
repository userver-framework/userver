#pragma once

#include <userver/dynamic_config/fwd.hpp>
#include <userver/dynamic_config/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace taxi_config {

template <typename T>
using Value = dynamic_config::Value<T>;

template <typename ValueType>
using ValueDict = dynamic_config::ValueDict<ValueType>;

using Snapshot = dynamic_config::Snapshot;
using DocsMap = dynamic_config::DocsMap;
using KeyValue = dynamic_config::KeyValue;
using dynamic_config::kValueDictDefaultName;

}  // namespace taxi_config

USERVER_NAMESPACE_END
