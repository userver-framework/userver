#pragma once

/// @file userver/storages/secdist/default_provider.hpp
/// @brief @copybrief storages::secdist::DefaultProvider

#include <string>

#include <userver/storages/secdist/provider.hpp>
#include <userver/storages/secdist/secdist.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::secdist {
// clang-format off

/// @brief Default implementation of storages::secdist::SecdistProvider.
///
/// Provides components::Secdist configurations to the
/// storages::secdist::SecdistConfig

// clang-format on
/// [Sample secdist - default provider]
class DefaultProvider final : public storages::secdist::SecdistProvider {
 public:
  struct Settings {
    std::string config_path;
    SecdistFormat format{SecdistFormat::kJson};
    bool missing_ok{false};
    std::optional<std::string> environment_secrets_key;
    engine::TaskProcessor* blocking_task_processor{nullptr};
  };

  explicit DefaultProvider(Settings settings);

  formats::json::Value Get() const override;

 private:
  Settings settings_;
};
/// [Sample secdist - default provider]
}  // namespace storages::secdist

USERVER_NAMESPACE_END
