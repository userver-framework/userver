#include <utest/taxi_config.hpp>

#include <fs/blocking/read.hpp>
#include <taxi_config/storage_mock.hpp>

namespace utest {

namespace {

taxi_config::DocsMap ReadDefaultDocsMap(const std::string& filename) {
  auto contents = fs::blocking::ReadFileContents(filename);
  ::taxi_config::DocsMap docs_map;
  docs_map.Parse(contents, false);
  return docs_map;
}

}  // namespace

namespace impl {

taxi_config::Source ReadDefaultTaxiConfigSource(const std::string& filename) {
  static const taxi_config::StorageMock storage(ReadDefaultDocsMap(filename));
  return *storage;
}

std::shared_ptr<const taxi_config::Config> ReadDefaultTaxiConfigPtr(
    const std::string& filename) {
  static const auto snapshot =
      ReadDefaultTaxiConfigSource(filename).GetSnapshot();
  return {std::shared_ptr<void>{}, &*snapshot};
}

}  // namespace impl

utils::SharedReadablePtr<taxi_config::Config> MakeTaxiConfigPtr(
    const taxi_config::DocsMap& docs_map) {
  return std::make_shared<const taxi_config::Config>(
      taxi_config::Config::Parse(docs_map));
}

}  // namespace utest
