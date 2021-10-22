#pragma once

#include <string>
#include <vector>

#include <userver/taxi_config/source.hpp>
#include <userver/taxi_config/storage_mock.hpp>

USERVER_NAMESPACE_BEGIN

namespace taxi_config::impl {

// Internal function, use taxi_config::GetDefaultSource() instead
taxi_config::Source GetDefaultSource(const std::string& filename);

// Internal function, use taxi_config::GetDefaultSnapshot() instead
const taxi_config::Snapshot& GetDefaultSnapshot(const std::string& filename);

// Internal function, use taxi_config::MakeDefaultStorage() instead
taxi_config::StorageMock MakeDefaultStorage(
    const std::string& filename,
    const std::vector<taxi_config::KeyValue>& overrides);

}  // namespace taxi_config::impl

USERVER_NAMESPACE_END
