#pragma once

/// @file userver/storages/secdist/provider.hpp

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::secdist {

class SecdistProvider {
 public:
  SecdistProvider() = default;
  virtual ~SecdistProvider() = default;

  SecdistProvider(const SecdistProvider&) = delete;
  SecdistProvider& operator=(const SecdistProvider&) = delete;

  SecdistProvider(SecdistProvider&&) = default;
  SecdistProvider& operator=(SecdistProvider&&) = default;

  virtual formats::json::Value Get() const = 0;
};

}  // namespace storages::secdist

USERVER_NAMESPACE_END
