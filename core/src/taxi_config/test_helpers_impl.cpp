#include <taxi_config/test_helpers_impl.hpp>

#include <fs/blocking/read.hpp>

namespace taxi_config::impl {
namespace {

taxi_config::DocsMap ReadDefaultDocsMap(const std::string& filename) {
  auto contents = fs::blocking::ReadFileContents(filename);
  ::taxi_config::DocsMap docs_map;
  docs_map.Parse(contents, false);
  return docs_map;
}

}  // namespace

const taxi_config::DocsMap& GetDefaultDocsMap(const std::string& filename) {
  static const auto default_docs_map = ReadDefaultDocsMap(filename);
  return default_docs_map;
}

taxi_config::Source GetDefaultSource(const std::string& filename) {
  static const auto storage = MakeDefaultStorage(filename, {});
  return storage.GetSource();
}

taxi_config::StorageMock MakeDefaultStorage(
    const std::string& filename,
    const std::vector<taxi_config::KeyValue>& overrides) {
  return taxi_config::StorageMock(GetDefaultDocsMap(filename), overrides);
}

}  // namespace taxi_config::impl
