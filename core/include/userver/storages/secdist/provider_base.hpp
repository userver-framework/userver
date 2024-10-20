#pragma once

/// @file userver/storages/secdist/provider_base.hpp

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::secdist {

class SecdistProviderBase {
 public:
  SecdistProviderBase() = default;
  virtual ~SecdistProviderBase() = default;

  SecdistProviderBase(const SecdistProviderBase&) = delete;
  SecdistProviderBase& operator=(const SecdistProviderBase&) = delete;

  SecdistProviderBase(SecdistProviderBase&&) = default;
  SecdistProviderBase& operator=(SecdistProviderBase&&) = default;

  virtual formats::json::Value Get() const = 0;
};

}  // namespace storages::secdist

USERVER_NAMESPACE_END
