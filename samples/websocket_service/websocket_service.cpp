#include <userver/server/websocket/websocket_server.h>
#include <userver/components/loggable_component_base.hpp>
#include <userver/components/manager_controller_component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/components/tracer.hpp>
#include <userver/concurrent/background_task_storage.hpp>
#include <userver/dynamic_config/fallbacks/component.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/logging/component.hpp>
#include <userver/os_signals/component.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/daemon_run.hpp>

using namespace userver;

void RunEchoCoro(std::shared_ptr<websocket::WebSocketConnection> connection) {
  auto messageSrc = connection->GetMessagesConsumer();

  while (!engine::current_task::ShouldCancel()) {
    websocket::Message message;
    if (messageSrc.Pop(message, {})) {
      LOG_INFO() << fmt::format(
          "got message isText {}, closed {}, data size {}", message.isText,
          message.closed, message.data.size());
      if (message.closed) break;
      connection->Send(std::move(message));
    } else {
      break;
    }
  }
  connection->Close((int)websocket::CloseStatus::kGoingAway);
}

class ServiceComponent final : public websocket::WebSocketServer {
 public:
  static constexpr std::string_view kName = "ws-echo-server";

  ServiceComponent(const components::ComponentConfig& component_config,
                   const components::ComponentContext& component_context)
      : websocket::WebSocketServer(component_config, component_context) {}

 private:
  void onNewWSConnection(
      std::shared_ptr<websocket::WebSocketConnection> connection) override {
    LOG_INFO() << "New websocket connection from " << connection->RemoteAddr();
    bts.AsyncDetach("ws-echo", &RunEchoCoro, connection);
  }

  userver::concurrent::BackgroundTaskStorage bts;
};

int main(int argc, char* argv[]) {
  auto component_list = components::ComponentList()
                            .Append<os_signals::ProcessorComponent>()
                            .Append<components::Logging>()
                            .Append<components::Tracer>()
                            .Append<components::ManagerControllerComponent>()
                            .Append<components::StatisticsStorage>()
                            .Append<components::DynamicConfig>()
                            .Append<components::DynamicConfigFallbacks>()
                            .Append<ServiceComponent>();

  return utils::DaemonMain(argc, argv, component_list);
}
