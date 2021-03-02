#include <utest/taxi_config.hpp>

#include <fs/blocking/read.hpp>

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

std::shared_ptr<const taxi_config::Config> ReadDefaultTaxiConfigPtr(
    const std::string& filename) {
  static const auto config = std::make_shared<const taxi_config::Config>(
      taxi_config::Config::Parse(ReadDefaultDocsMap(filename)));
  return config;
}

const taxi_config::StorageMock& ReadDefaultTaxiConfigStorage(
    const std::string& filename) {
  static const taxi_config::StorageMock storage(ReadDefaultDocsMap(filename));
  return storage;
}

}  // namespace impl

utils::SharedReadablePtr<taxi_config::Config> MakeTaxiConfigPtr(
    const taxi_config::DocsMap& docs_map) {
  return std::make_shared<const taxi_config::Config>(
      taxi_config::Config::Parse(docs_map));
}

}  // namespace utest
