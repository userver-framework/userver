#pragma once

#include <userver/dynamic_config/storage_mock.hpp>

USERVER_NAMESPACE_BEGIN

namespace taxi_config {

using StorageMock = dynamic_config::StorageMock;
using KeyValue = dynamic_config::KeyValue;

}  // namespace taxi_config

USERVER_NAMESPACE_END
