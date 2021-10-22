#include <userver/taxi_config/test_helpers_impl.hpp>

#include <userver/fs/blocking/read.hpp>

USERVER_NAMESPACE_BEGIN

namespace taxi_config::impl {
namespace {

taxi_config::DocsMap ReadDefaultDocsMap(const std::string& filename) {
  auto contents = fs::blocking::ReadFileContents(filename);
  taxi_config::DocsMap docs_map;
  docs_map.Parse(contents, false);
  return docs_map;
}

const taxi_config::DocsMap& GetDefaultDocsMap(const std::string& filename) {
  static const auto default_docs_map = ReadDefaultDocsMap(filename);
  return default_docs_map;
}

}  // namespace

taxi_config::Source GetDefaultSource(const std::string& filename) {
  static const auto storage = MakeDefaultStorage(filename, {});
  return storage.GetSource();
}

const taxi_config::Snapshot& GetDefaultSnapshot(const std::string& filename) {
  static const auto snapshot = GetDefaultSource(filename).GetSnapshot();
  return snapshot;
}

taxi_config::StorageMock MakeDefaultStorage(
    const std::string& filename,
    const std::vector<taxi_config::KeyValue>& overrides) {
  return taxi_config::StorageMock(GetDefaultDocsMap(filename), overrides);
}

}  // namespace taxi_config::impl

USERVER_NAMESPACE_END
