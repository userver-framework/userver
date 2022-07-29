#include <userver/utest/using_namespace_userver.hpp>

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
#include <userver/testsuite/testpoint.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/rabbitmq.hpp>

namespace samples::urabbitmq {

class MyRabbitComponent final : public components::RabbitMQ {
 public:
  static constexpr const char* kName = "my-rabbit";

  using MessagesStorage =
      userver::concurrent::Variable<std::vector<std::string>>;

  MyRabbitComponent(const components::ComponentConfig& config,
                    const components::ComponentContext& context)
      : components::RabbitMQ{config, context},
        client_{GetClient()},
        consumer_{client_, queue_, consumed_messages_} {
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

  std::vector<std::string> GetConsumedMessages() {
    auto storage = consumed_messages_.Lock();
    auto messages = *storage;
    std::sort(messages.begin(), messages.end());

    return messages;
  }

  class Consumer final : public userver::urabbitmq::ConsumerBase {
   public:
    Consumer(std::shared_ptr<userver::urabbitmq::Client> client,
             const userver::urabbitmq::Queue& queue, MessagesStorage& storage)
        : userver::urabbitmq::ConsumerBase{std::move(client), {queue, 10}},
          storage_{storage} {
      Start();
    };

    ~Consumer() override { Stop(); }

    void Process(std::string message) override {
      auto storage = storage_.Lock();
      storage->push_back(std::move(message));

      TESTPOINT("message_consumed", {});
    }

   private:
    MessagesStorage& storage_;
  };

 private:
  const userver::urabbitmq::Exchange exchange_{"sample-exchange"};
  const userver::urabbitmq::Queue queue_{"sample-queue"};
  const std::string routing_key_ = "sample-routing-key";

  std::shared_ptr<userver::urabbitmq::Client> client_;
  MessagesStorage consumed_messages_;
  Consumer consumer_;
};

class RequestHandler final : public server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr const char* kName = "my-http-handler";
  RequestHandler(const components::ComponentConfig& config,
                 const components::ComponentContext& context)
      : server::handlers::HttpHandlerJsonBase{config, context},
        my_rabbit_{context.FindComponent<MyRabbitComponent>()} {}
  ~RequestHandler() override = default;

  formats::json::Value HandleRequestJsonThrow(
      const server::http::HttpRequest& request,
      const formats::json::Value& request_json,
      server::request::RequestContext&) const override {
    if (request.GetMethod() == userver::server::http::HttpMethod::kGet) {
      formats::json::ValueBuilder builder{formats::json::Type::kObject};
      builder["messages"] = my_rabbit_.GetConsumedMessages();

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
};

}  // namespace samples::urabbitmq

int main(int argc, char* argv[]) {
  const auto components_list =
      userver::components::MinimalServerComponentList()
          .Append<samples::urabbitmq::MyRabbitComponent>()
          .Append<samples::urabbitmq::RequestHandler>()
          .Append<userver::clients::dns::Component>()
          .Append<userver::components::Secdist>()
          .Append<userver::components::TestsuiteSupport>()
          .Append<userver::server::handlers::TestsControl>()
          .Append<components::HttpClient>();

  return userver::utils::DaemonMain(argc, argv, components_list);
}
