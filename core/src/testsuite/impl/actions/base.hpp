#pragma once

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite::impl::actions {

class BaseTestsuiteAction {
 public:
  virtual ~BaseTestsuiteAction() = default;
  virtual formats::json::Value Perform(
      const formats::json::Value& request_body) const = 0;
};

}  // namespace testsuite::impl::actions

USERVER_NAMESPACE_END
