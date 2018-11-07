#pragma once

#include <components/component_base.hpp>

namespace components {

class HttpServerSettingsBase : public LoggableComponentBase {
 public:
  using LoggableComponentBase::LoggableComponentBase;

  virtual ~HttpServerSettingsBase() = default;

  static constexpr const char* kName = "http-server-settings";

  virtual bool NeedLogRequest() const = 0;
  virtual bool NeedLogRequestHeaders() const = 0;
};

}  // namespace components
