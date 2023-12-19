#include <userver/utest/using_namespace_userver.hpp>

#include <vector>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/rabbitmq.hpp>

namespace chaos {

class ChaosRabbit final : public components::RabbitMQ {
 public:
  static constexpr std::string_view kName{"chaos-rabbit"};

  ChaosRabbit(const components::ComponentConfig& config,
              const components::ComponentContext& context)
      : components::RabbitMQ{config, context}, rabbit_client_{GetClient()} {
    const auto setup_deadline =
        engine::Deadline::FromDuration(kDefaultOperationTimeout);

    auto admin_channel = rabbit_client_->GetAdminChannel(setup_deadline);
    admin_channel.DeclareExchange(exchange_, urabbitmq::Exchange::Type::kFanOut,
                                  setup_deadline);
    admin_channel.DeclareQueue(queue_, setup_deadline);
    admin_channel.BindQueue(exchange_, queue_, routing_key_, setup_deadline);
  }

  ~ChaosRabbit() override {
    const auto teardown_deadline =
        engine::Deadline::FromDuration(kDefaultOperationTimeout);

    try {
      auto admin_channel = rabbit_client_->GetAdminChannel(teardown_deadline);
      admin_channel.RemoveQueue(queue_, teardown_deadline);
      admin_channel.RemoveExchange(exchange_, teardown_deadline);
    } catch (const std::exception& ex) {
      LOG_ERROR() << "Failed to clean up rabbitmq infrastructure: " << ex;
    }
  }

  void PublishReliable(const std::string& message) const {
    rabbit_client_->PublishReliable(
        exchange_, routing_key_, message, urabbitmq::MessageType::kTransient,
        engine::Deadline::FromDuration(kDefaultOperationTimeout));
  }

  void PublishUnreliable(const std::string& message) const {
    rabbit_client_->Publish(
        exchange_, routing_key_, message, urabbitmq::MessageType::kTransient,
        engine::Deadline::FromDuration(kDefaultOperationTimeout));
  }

 private:
  static constexpr std::chrono::milliseconds kDefaultOperationTimeout{2000};

  const urabbitmq::Exchange exchange_{"chaos-exchange"};
  const urabbitmq::Queue queue_{"chaos-queue"};
  const std::string routing_key_ = "chaos-routing-key";

  const std::shared_ptr<urabbitmq::Client> rabbit_client_;
};

class ChaosConsumer final : public components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName{"chaos-consumer"};

  ChaosConsumer(const components::ComponentConfig& config,
                const components::ComponentContext& context)
      : components::LoggableComponentBase{config, context},
        consumer_{config, context, messages_} {
    Start();
  }

  ~ChaosConsumer() override { Stop(); }

  void Start() { consumer_.Start(); }
  void Stop() { consumer_.Stop(); }

  static yaml_config::Schema GetStaticConfigSchema() {
    return urabbitmq::ConsumerComponentBase::GetStaticConfigSchema();
  }

  std::vector<std::string> GetMessages() const {
    auto messages = [this] {
      auto storage = messages_.Lock();
      return *storage;
    }();

    std::sort(messages.begin(), messages.end());
    return messages;
  }

  void Clear() {
    auto storage = messages_.Lock();
    storage->clear();
  }

 private:
  class Consumer final : public urabbitmq::ConsumerBase {
   public:
    Consumer(const components::ComponentConfig& config,
             const components::ComponentContext& context,
             concurrent::Variable<std::vector<std::string>>& messages)
        : urabbitmq::ConsumerBase{context
                                      .FindComponent<components::RabbitMQ>(
                                          config["rabbit_name"]
                                              .As<std::string>())
                                      .GetClient(),
                                  ParseSettings(config)},
          messages_{messages} {}

   protected:
    void Process(std::string message) override {
      {
        auto storage = messages_.Lock();
        storage->push_back(std::move(message));
      }
      TESTPOINT("message_consumed", {});
    }

   private:
    urabbitmq::ConsumerSettings ParseSettings(
        const components::ComponentConfig& config) {
      return {urabbitmq::Queue{config["queue"].As<std::string>()},
              config["prefetch_count"].As<std::uint16_t>()};
    }

    concurrent::Variable<std::vector<std::string>>& messages_;
  };

  concurrent::Variable<std::vector<std::string>> messages_;
  Consumer consumer_;
};

class ChaosHandler final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName{"chaos-handler"};

  ChaosHandler(const components::ComponentConfig& config,
               const components::ComponentContext& context)
      : server::handlers::HttpHandlerBase{config, context},
        rabbit_{context.FindComponent<ChaosRabbit>()},
        consumer_{context.FindComponent<ChaosConsumer>()} {}

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&) const override {
    switch (request.GetMethod()) {
      case server::http::HttpMethod::kPost:
        return HandlePost(request);
      case server::http::HttpMethod::kPatch:
        return HandlePatch(request);
      case server::http::HttpMethod::kGet:
        return HandleGet();
      case server::http::HttpMethod::kDelete:
        return HandleDelete();
      default:
        throw server::handlers::ClientError{server::handlers::ExternalBody{
            fmt::format("Unsupported method {}", request.GetMethod())}};
    }
  }

 private:
  std::string HandlePost(const server::http::HttpRequest& request) const {
    const auto& message = request.GetArg("message");
    if (message.empty()) {
      throw server::handlers::ClientError{
          server::handlers::ExternalBody{"No 'message' query argument"}};
    }

    const auto& reliable = request.GetArg("reliable");
    if (!reliable.empty()) {
      rabbit_.PublishReliable(message);
    } else {
      rabbit_.PublishUnreliable(message);
    }

    return {};
  }

  std::string HandlePatch(const server::http::HttpRequest& request) const {
    const auto& action = request.GetArg("action");
    if (action == "start") {
      consumer_.Start();
    } else if (action == "stop") {
      consumer_.Stop();
    } else {
      throw server::handlers::ClientError{server::handlers::ExternalBody{
          fmt::format("Unknown action '{}'", action)}};
    }

    return {};
  }

  std::string HandleGet() const {
    const auto messages_list = consumer_.GetMessages();

    return formats::json::ToString(
        formats::json::ValueBuilder{messages_list}.ExtractValue());
  }

  std::string HandleDelete() const {
    consumer_.Clear();

    return {};
  }

  const ChaosRabbit& rabbit_;
  ChaosConsumer& consumer_;
};

}  // namespace chaos

int main(int argc, char* argv[]) {
  const auto component_list = components::MinimalServerComponentList()
                                  .Append<chaos::ChaosRabbit>()
                                  .Append<chaos::ChaosConsumer>()
                                  .Append<chaos::ChaosHandler>()
                                  //
                                  .Append<clients::dns::Component>()
                                  .Append<components::Secdist>()
                                  .Append<components::DefaultSecdistProvider>()
                                  //
                                  .Append<server::handlers::TestsControl>()
                                  .Append<components::TestsuiteSupport>()
                                  .Append<components::HttpClient>();

  return utils::DaemonMain(argc, argv, component_list);
}
