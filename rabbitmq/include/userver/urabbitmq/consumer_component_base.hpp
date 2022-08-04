#pragma once

#include <userver/components/loggable_component_base.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class ConsumerComponentBase : public components::LoggableComponentBase {
 public:
  ConsumerComponentBase(const components::ComponentConfig& config,
                        const components::ComponentContext& context);
  ~ConsumerComponentBase() override;

  static yaml_config::Schema GetStaticConfigSchema();

 protected:
  void Start();

  void Stop();

  virtual void Process(std::string message) = 0;

 private:
  class Impl;
  utils::FastPimpl<Impl, 488, 8> impl_;
};

}  // namespace urabbitmq

namespace components {

template <>
inline constexpr bool kHasValidate<urabbitmq::ConsumerComponentBase> = true;

}

USERVER_NAMESPACE_END
