#include <userver/dump/factory.hpp>

#include <dump/secdist.hpp>
#include <userver/dump/operations_encrypted.hpp>
#include <userver/dump/operations_file.hpp>
#include <userver/storages/secdist/component.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

namespace {

boost::filesystem::perms GetPerms(const Config& config) {
  using boost::filesystem::perms;
  if (config.world_readable)
    return perms::owner_read | perms::group_read | perms::others_read;
  else
    return perms::owner_read;
}

}  // namespace

std::unique_ptr<dump::OperationsFactory> CreateOperationsFactory(
    const Config& config, const components::ComponentContext& context) {
  auto dump_perms = GetPerms(config);

  if (config.dump_is_encrypted) {
    const auto& secdist = context.FindComponent<components::Secdist>().Get();
    auto secret_key = secdist.Get<dump::Secdist>().GetSecretKey(config.name);
    return std::make_unique<dump::EncryptedOperationsFactory>(
        std::move(secret_key), dump_perms);
  } else {
    return std::make_unique<dump::FileOperationsFactory>(dump_perms);
  }
}

std::unique_ptr<dump::OperationsFactory> CreateDefaultOperationsFactory(
    const Config& config) {
  auto dump_perms = GetPerms(config);
  return std::make_unique<dump::FileOperationsFactory>(dump_perms);
}

}  // namespace dump

USERVER_NAMESPACE_END
