#pragma once

#include <userver/dynamic_config/test_helpers.hpp>
#include <userver/taxi_config/storage_mock.hpp>

USERVER_NAMESPACE_BEGIN

namespace taxi_config {

#if defined(DEFAULT_TAXI_CONFIG_FILENAME) || defined(DOXYGEN)
using dynamic_config::GetDefaultSource;

using dynamic_config::GetDefaultSnapshot;

using dynamic_config::MakeDefaultStorage;

#endif

}  // namespace taxi_config

USERVER_NAMESPACE_END
