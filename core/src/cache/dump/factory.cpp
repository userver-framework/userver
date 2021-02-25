#include <cache/dump/factory.hpp>

#include <cache/dump/operations_encrypted.hpp>
#include <cache/dump/operations_file.hpp>
#include <cache/dump/secdist.hpp>
#include <storages/secdist/component.hpp>

namespace cache::dump {

namespace {

boost::filesystem::perms GetPerms(const CacheConfigStatic& config) {
  using boost::filesystem::perms;
  if (config.world_readable)
    return perms::owner_read | perms::group_read | perms::others_read;
  else
    return perms::owner_read;
}

}  // namespace

std::unique_ptr<dump::OperationsFactory> CreateOperationsFactory(
    const CacheConfigStatic& config,
    const components::ComponentContext& context,
    const std::string& cache_name) {
  auto dump_perms = GetPerms(config);

  if (config.dump_is_encrypted) {
    const auto& secdist = context.FindComponent<components::Secdist>().Get();
    auto secret_key = secdist.Get<dump::Secdist>().GetSecretKey(cache_name);
    return std::make_unique<dump::EncryptedOperationsFactory>(
        std::move(secret_key), dump_perms);
  } else {
    return std::make_unique<dump::FileOperationsFactory>(dump_perms);
  }
}

std::unique_ptr<dump::OperationsFactory> CreateDefaultOperationsFactory(
    const CacheConfigStatic& config) {
  auto dump_perms = GetPerms(config);
  return std::make_unique<dump::FileOperationsFactory>(dump_perms);
}

}  // namespace cache::dump
