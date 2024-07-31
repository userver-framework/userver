#pragma once

#include <pytypedefs.h>

#include <userver/components/component_base.hpp>
#include <userver/engine/mutex.hpp>

namespace upython {

class Component final : public userver::components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName{"upython"};

  Component(const userver::components::ComponentConfig& config,
            const userver::components::ComponentContext& context);


  std::string RunScript();

 private:
  PyObject *pModule_{nullptr};
  userver::engine::Mutex mutex_;
};

}  // namespace upython
