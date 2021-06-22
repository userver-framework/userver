#include <utest/taxi_config.hpp>

#include <fs/blocking/read.hpp>
#include <taxi_config/storage_mock.hpp>

namespace utest::impl {
namespace {

struct StorageAndConfig final {
  explicit StorageAndConfig(const taxi_config::DocsMap& docs_map)
      : storage(docs_map, {}), snapshot(storage.GetSnapshot()) {}

  taxi_config::StorageMock storage;
  taxi_config::Snapshot snapshot;
};

}  // namespace

std::shared_ptr<const taxi_config::Snapshot> ReadDefaultTaxiConfigPtr(
    const std::string& filename) {
  static const auto config =
      taxi_config::impl::GetDefaultSource(filename).GetSnapshot();
  return {std::shared_ptr<void>{}, &config};
}

utils::SharedReadablePtr<taxi_config::Snapshot> MakeTaxiConfigPtr(
    const std::string& filename, const taxi_config::DocsMap& overrides) {
  auto merged = taxi_config::impl::GetDefaultDocsMap(filename);
  merged.MergeFromOther(taxi_config::DocsMap{overrides});

  auto storage = std::make_shared<StorageAndConfig>(merged);
  const auto& config = storage->snapshot;
  // NOLINTNEXTLINE(hicpp-move-const-arg)
  return std::shared_ptr<const taxi_config::Snapshot>{std::move(storage),
                                                      &config};
}

}  // namespace utest::impl
