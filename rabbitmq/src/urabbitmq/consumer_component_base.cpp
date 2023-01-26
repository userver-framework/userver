#include <userver/urabbitmq/consumer_component_base.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/urabbitmq/component.hpp>
#include <userver/urabbitmq/consumer_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

ConsumerSettings Parse(const yaml_config::YamlConfig& config,
                       formats::parse::To<ConsumerSettings>) {
  ConsumerSettings settings;
  settings.queue = Queue{config["queue"].As<std::string>()};
  settings.prefetch_count = config["prefetch_count"].As<uint16_t>();

  UINVARIANT(settings.prefetch_count > 0, "prefetch_count is set to zero");

  return settings;
}

class ConsumerComponentBase::Impl final : public ConsumerBase {
 public:
  Impl(std::shared_ptr<Client>&& client, const ConsumerSettings& settings)
      : ConsumerBase{std::move(client), settings} {}

  ~Impl() override = default;

  void Start(ConsumerComponentBase* parent) {
    parent_ = parent;
    ConsumerBase::Start();
  }

 protected:
  void Process(std::string message) override {
    UASSERT(parent_ != nullptr);
    parent_->Process(std::move(message));
  }

 private:
  ConsumerComponentBase* parent_{nullptr};
};

ConsumerComponentBase::ConsumerComponentBase(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : components::LoggableComponentBase{config, context},
      impl_{std::make_unique<Impl>(
          context
              .FindComponent<components::RabbitMQ>(
                  config["rabbit_name"].As<std::string>())
              .GetClient(),
          config.As<ConsumerSettings>())} {}

ConsumerComponentBase::~ConsumerComponentBase() = default;

void ConsumerComponentBase::OnAllComponentsLoaded() { impl_->Start(this); }

void ConsumerComponentBase::OnAllComponentsAreStopping() { impl_->Stop(); }

yaml_config::Schema ConsumerComponentBase::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<components::LoggableComponentBase>(R"(
type: object
description: RabbitMQ consumer component
additionalProperties: false
properties:
    rabbit_name:
        type: string
        description: name of the RabbitMQ component to use
    queue:
        type: string
        description: a queue to consume from
    prefetch_count:
        type: integer
        description: prefetch_count for the consumer
)");
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
