#include <utest/taxi_config.hpp>

#include <fs/blocking/read.hpp>
#include <taxi_config/storage_mock.hpp>

namespace utest::impl {

namespace {

taxi_config::DocsMap ReadDefaultDocsMap(const std::string& filename) {
  auto contents = fs::blocking::ReadFileContents(filename);
  ::taxi_config::DocsMap docs_map;
  docs_map.Parse(contents, false);
  return docs_map;
}

const taxi_config::DocsMap& GetDefaultDocsMap(const std::string& filename) {
  static const auto default_docs_map = ReadDefaultDocsMap(filename);
  return default_docs_map;
}

}  // namespace

taxi_config::Source ReadDefaultTaxiConfigSource(const std::string& filename) {
  static const taxi_config::StorageMock storage(GetDefaultDocsMap(filename));
  return *storage;
}

std::shared_ptr<const taxi_config::Config> ReadDefaultTaxiConfigPtr(
    const std::string& filename) {
  static const auto snapshot =
      ReadDefaultTaxiConfigSource(filename).GetSnapshot();
  return {std::shared_ptr<void>{}, &*snapshot};
}

utils::SharedReadablePtr<taxi_config::Config> MakeTaxiConfigPtr(
    const std::string& filename, const taxi_config::DocsMap& overrides) {
  auto merged = GetDefaultDocsMap(filename);
  merged.MergeFromOther(taxi_config::DocsMap{overrides});
  return std::make_shared<const taxi_config::Config>(
      taxi_config::Config::Parse(merged));
}

}  // namespace utest::impl
