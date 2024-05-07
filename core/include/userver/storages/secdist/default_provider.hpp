#pragma once

/// @file userver/storages/secdist/component.hpp
/// @brief @copybrief components::DefaultSecdistProvider

#include <string>

#include <userver/storages/secdist/provider.hpp>
#include <userver/storages/secdist/secdist.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::secdist {

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

}  // namespace storages::secdist

USERVER_NAMESPACE_END
