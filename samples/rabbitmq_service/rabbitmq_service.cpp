#include <userver/utest/using_namespace_userver.hpp>

#include <string_view>
#include <vector>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/formats/json/serialize_container.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/rabbitmq.hpp>

namespace samples::amqp {

class MyRabbitComponent final : public components::RabbitMQ {
 public:
  static constexpr std::string_view kName{"my-rabbit"};

  MyRabbitComponent(const components::ComponentConfig& config,
                    const components::ComponentContext& context)
      : components::RabbitMQ{config, context}, client_{GetClient()} {
    const auto setup_deadline =
        engine::Deadline::FromDuration(std::chrono::seconds{2});

    auto admin_channel = client_->GetAdminChannel(setup_deadline);
    admin_channel.DeclareExchange(exchange_, urabbitmq::Exchange::Type::kFanOut,
                                  setup_deadline);
    admin_channel.DeclareQueue(queue_, setup_deadline);
    admin_channel.BindQueue(exchange_, queue_, routing_key_, setup_deadline);
  }

  ~MyRabbitComponent() override {
    auto admin_channel = client_->GetAdminChannel(
        engine::Deadline::FromDuration(std::chrono::seconds{1}));

    const auto teardown_deadline =
        engine::Deadline::FromDuration(std::chrono::seconds{2});
    admin_channel.RemoveQueue(queue_, teardown_deadline);
    admin_channel.RemoveExchange(exchange_, teardown_deadline);
  }

  void Publish(const std::string& message) {
    client_->PublishReliable(
        exchange_, routing_key_, message, urabbitmq::MessageType::kTransient,
        engine::Deadline::FromDuration(std::chrono::seconds{2}));
  }

 private:
  const urabbitmq::Exchange exchange_{"sample-exchange"};
  const urabbitmq::Queue queue_{"sample-queue"};
  const std::string routing_key_ = "sample-routing-key";

  std::shared_ptr<urabbitmq::Client> client_;
};

class MyRabbitConsumer final : public urabbitmq::ConsumerComponentBase {
 public:
  static constexpr std::string_view kName{"my-consumer"};

  MyRabbitConsumer(const components::ComponentConfig& config,
                   const components::ComponentContext& context)
      : urabbitmq::ConsumerComponentBase{config, context} {}

  std::vector<int> GetConsumedMessages() {
    auto storage = storage_.Lock();

    auto messages = *storage;
    // We sort messages here because `Process` might run in parallel
    // and ordering is not guaranteed.
    std::sort(messages.begin(), messages.end());

    return messages;
  }

 protected:
  void Process(std::string message) override {
    const auto as_integer = std::stoi(message);

    auto storage = storage_.Lock();
    storage->push_back(as_integer);

    TESTPOINT("message_consumed", {});
  }

 private:
  concurrent::Variable<std::vector<int>> storage_;
};

class RequestHandler final : public server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr const char* kName = "my-http-handler";
  RequestHandler(const components::ComponentConfig& config,
                 const components::ComponentContext& context)
      : server::handlers::HttpHandlerJsonBase{config, context},
        my_rabbit_{context.FindComponent<MyRabbitComponent>()},
        my_consumer_{context.FindComponent<MyRabbitConsumer>()} {}
  ~RequestHandler() override = default;

  formats::json::Value HandleRequestJsonThrow(
      const server::http::HttpRequest& request,
      const formats::json::Value& request_json,
      server::request::RequestContext&) const override {
    if (request.GetMethod() == server::http::HttpMethod::kGet) {
      formats::json::ValueBuilder builder{formats::json::Type::kObject};
      builder["messages"] = my_consumer_.GetConsumedMessages();

      return builder.ExtractValue();
    } else {
      if (!request_json.HasMember("message")) {
        request.SetResponseStatus(server::http::HttpStatus::kBadRequest);
        return formats::json::FromString(
            R"("{"error": "missing required field "message""}")");
      }

      my_rabbit_.Publish(request_json["message"].As<std::string>());

      return {};
    }
  }

 private:
  MyRabbitComponent& my_rabbit_;
  MyRabbitConsumer& my_consumer_;
};

}  // namespace samples::amqp

int main(int argc, char* argv[]) {
  const auto components_list = components::MinimalServerComponentList()
                                   .Append<samples::amqp::MyRabbitComponent>()
                                   .Append<samples::amqp::MyRabbitConsumer>()
                                   .Append<samples::amqp::RequestHandler>()
                                   .Append<clients::dns::Component>()
                                   .Append<components::Secdist>()
                                   .Append<components::DefaultSecdistProvider>()
                                   .Append<components::TestsuiteSupport>()
                                   .Append<server::handlers::TestsControl>()
                                   .Append<components::HttpClient>();

  return utils::DaemonMain(argc, argv, components_list);
}
