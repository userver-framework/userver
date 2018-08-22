#pragma once

#include <yandex/taxi/userver/components/component_base.hpp>

namespace components {

class HttpServerSettingsBase : public ComponentBase {
 public:
  virtual ~HttpServerSettingsBase() = default;

  static constexpr const char* kName = "http-server-settings";

  virtual bool NeedLogRequest() const = 0;
  virtual bool NeedLogRequestHeaders() const = 0;
};

}  // namespace components
