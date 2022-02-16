#pragma once

#include <string>
#include <vector>

#include <userver/dynamic_config/source.hpp>
#include <userver/dynamic_config/storage_mock.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config::impl {

// Internal function, use dynamic_config::GetDefaultSource() instead
dynamic_config::Source GetDefaultSource(const std::string& filename);

// Internal function, use dynamic_config::GetDefaultSnapshot() instead
const dynamic_config::Snapshot& GetDefaultSnapshot(const std::string& filename);

// Internal function, use dynamic_config::MakeDefaultStorage() instead
dynamic_config::StorageMock MakeDefaultStorage(
    const std::string& filename,
    const std::vector<dynamic_config::KeyValue>& overrides);

}  // namespace dynamic_config::impl

USERVER_NAMESPACE_END
