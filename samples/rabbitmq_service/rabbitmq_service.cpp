#include <userver/utest/using_namespace_userver.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/rabbitmq.hpp>

namespace samples::urabbitmq {

class MyRabbitComponent final : public components::RabbitMQ {
 public:
  static constexpr const char* kName = "my-rabbit";

  MyRabbitComponent(const components::ComponentConfig& config,
                    const components::ComponentContext& context)
      : components::RabbitMQ{config, context}, client_{GetClient()} {
    auto admin_channel = client_->GetAdminChannel();

    const auto setup_deadline =
        userver::engine::Deadline::FromDuration(std::chrono::seconds{2});
    admin_channel.DeclareExchange(exchange_,
                                  userver::urabbitmq::ExchangeType::kFanOut, {},
                                  setup_deadline);
    admin_channel.DeclareQueue(queue_, {}, setup_deadline);
    admin_channel.BindQueue(exchange_, queue_, routing_key_, setup_deadline);
  }

  ~MyRabbitComponent() override {
    auto admin_channel = client_->GetAdminChannel();

    const auto teardown_deadline =
        userver::engine::Deadline::FromDuration(std::chrono::seconds{2});
    admin_channel.RemoveQueue(queue_, teardown_deadline);
    admin_channel.RemoveExchange(exchange_, teardown_deadline);
  }

  void Publish(const std::string& message) {
    auto channel = client_->GetReliableChannel();

    channel.Publish(
        exchange_, routing_key_, message,
        userver::urabbitmq::MessageType::kTransient,
        engine::Deadline::FromDuration(std::chrono::milliseconds{200}));
  }

 private:
  std::shared_ptr<userver::urabbitmq::Client> client_;

  const userver::urabbitmq::Exchange exchange_{"sample-exchange"};
  const userver::urabbitmq::Queue queue_{"sample-queue"};
  const std::string routing_key_ = "sample-routing-key";
};

class RequestHandler final : public server::handlers::HttpHandlerJsonBase {
 public:
  RequestHandler(const components::ComponentConfig& config,
                 const components::ComponentContext& context)
      : server::handlers::HttpHandlerJsonBase{config, context},
        my_rabbit_{context.FindComponent<MyRabbitComponent>()} {}
  ~RequestHandler() override = default;

  formats::json::Value HandleRequestJsonThrow(
      const server::http::HttpRequest& request,
      const formats::json::Value& request_json,
      server::request::RequestContext& context) const override {
    if (!request_json.HasMember("message")) {
      request.SetResponseStatus(server::http::HttpStatus::kBadRequest);
      return formats::json::FromString(
          R"("{"error": "missing required field "message""}")");
    }

    my_rabbit_.Publish(request_json["message"].As<std::string>());

    return {};
  }

 private:
  MyRabbitComponent& my_rabbit_;
};

int main(int argc, char* argv[]) {
  const auto components_list = userver::components::MinimalServerComponentList()
                                   .Append<MyRabbitComponent>()
                                   .Append<RequestHandler>("http-handler")
                                   .Append<userver::clients::dns::Component>()
                                   .Append<userver::components::Secdist>();

  return userver::utils::DaemonMain(argc, argv, components_list);
}

}  // namespace samples::urabbitmq