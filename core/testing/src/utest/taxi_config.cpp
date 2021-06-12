#include <utest/taxi_config.hpp>

#include <fs/blocking/read.hpp>
#include <taxi_config/storage_mock.hpp>

namespace utest::impl {

std::shared_ptr<const taxi_config::Config> ReadDefaultTaxiConfigPtr(
    const std::string& filename) {
  static const auto snapshot =
      taxi_config::impl::GetDefaultSource(filename).GetSnapshot();
  return {std::shared_ptr<void>{}, &*snapshot};
}

utils::SharedReadablePtr<taxi_config::Config> MakeTaxiConfigPtr(
    const std::string& filename, const taxi_config::DocsMap& overrides) {
  auto merged = taxi_config::impl::GetDefaultDocsMap(filename);
  merged.MergeFromOther(taxi_config::DocsMap{overrides});
  return std::make_shared<const taxi_config::Config>(
      taxi_config::Config::Parse(merged));
}

}  // namespace utest::impl
