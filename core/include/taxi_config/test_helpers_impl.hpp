#pragma once

#include <string>
#include <vector>

#include <taxi_config/source.hpp>
#include <taxi_config/storage_mock.hpp>

namespace taxi_config::impl {

// Internal function, don't use directly
const taxi_config::DocsMap& GetDefaultDocsMap(const std::string& filename);

// Internal function, use taxi_config::GetDefaultSource() instead
taxi_config::Source GetDefaultSource(const std::string& filename);

// Internal function, use taxi_config::GetDefaultSnapshot() instead
const taxi_config::Snapshot& GetDefaultSnapshot(const std::string& filename);

// Internal function, use taxi_config::MakeDefaultStorage() instead
taxi_config::StorageMock MakeDefaultStorage(
    const std::string& filename,
    const std::vector<taxi_config::KeyValue>& overrides);

}  // namespace taxi_config::impl
