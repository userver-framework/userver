#include <userver/dynamic_config/impl/test_helpers.hpp>

#include <userver/fs/blocking/read.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config::impl {
namespace {

dynamic_config::DocsMap ReadDefaultDocsMap(const std::string& filename) {
  auto contents = fs::blocking::ReadFileContents(filename);
  dynamic_config::DocsMap docs_map;
  docs_map.Parse(contents, false);
  return docs_map;
}

const dynamic_config::DocsMap& GetDefaultDocsMap(const std::string& filename) {
  static const auto default_docs_map = ReadDefaultDocsMap(filename);
  return default_docs_map;
}

}  // namespace

dynamic_config::Source GetDefaultSource(const std::string& filename) {
  static const auto storage = MakeDefaultStorage(filename, {});
  return storage.GetSource();
}

const dynamic_config::Snapshot& GetDefaultSnapshot(
    const std::string& filename) {
  static const auto snapshot = GetDefaultSource(filename).GetSnapshot();
  return snapshot;
}

dynamic_config::StorageMock MakeDefaultStorage(
    const std::string& filename,
    const std::vector<dynamic_config::KeyValue>& overrides) {
  return {GetDefaultDocsMap(filename), overrides};
}

}  // namespace dynamic_config::impl

USERVER_NAMESPACE_END
