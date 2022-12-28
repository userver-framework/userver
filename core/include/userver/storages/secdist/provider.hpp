#pragma once

/// @file storages/secdist/provider.hpp

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::secdist {

class SecdistProvider {
 public:
  virtual formats::json::Value Get() const = 0;
};

}  // namespace storages::secdist

USERVER_NAMESPACE_END
