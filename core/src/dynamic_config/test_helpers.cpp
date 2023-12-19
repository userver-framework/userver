#include <userver/dynamic_config/test_helpers.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config {

dynamic_config::Source GetDefaultSource() {
  static const auto storage = MakeDefaultStorage({});
  return storage.GetSource();
}

const dynamic_config::Snapshot& GetDefaultSnapshot() {
  static const auto snapshot = GetDefaultSource().GetSnapshot();
  return snapshot;
}

dynamic_config::StorageMock MakeDefaultStorage(
    const std::vector<dynamic_config::KeyValue>& overrides) {
  return {impl::GetDefaultDocsMap(), overrides};
}

namespace impl {

const dynamic_config::DocsMap& GetDefaultDocsMap() {
  static const auto default_docs_map =
      dynamic_config::impl::MakeDefaultDocsMap();
  return default_docs_map;
}

}  // namespace impl

}  // namespace dynamic_config

USERVER_NAMESPACE_END
