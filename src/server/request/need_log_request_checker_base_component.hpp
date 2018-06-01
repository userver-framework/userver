#pragma once

#include <components/component_base.hpp>

namespace components {

class NeedLogRequestCheckerBase : public ComponentBase {
 public:
  virtual ~NeedLogRequestCheckerBase() {}

  static constexpr const char* const kName = "need-log-request-checker";

  virtual bool NeedLogRequest() const = 0;
  virtual bool NeedLogRequestHeaders() const = 0;
};

}  // namespace components
