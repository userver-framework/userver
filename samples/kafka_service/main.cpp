#include <userver/components/minimal_server_component_list.hpp>

#include <userver/kafka/consumer_component.hpp>
#include <userver/kafka/producer_component.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>

#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>

#include <userver/utils/daemon_run.hpp>

#include <consumer_handler.hpp>
#include <producer_handler.hpp>

/// [Kafka service sample - main]
int main(int argc, char* argv[]) {
  const auto components_list =
      components::MinimalServerComponentList()
          .Append<kafka_sample::ConsumerHandler>()
          .Append<kafka_sample::ProducerHandler>()
          .Append<kafka::ConsumerComponent>("kafka-consumer")
          .Append<kafka::ProducerComponent>("kafka-producer")
          .Append<components::TestsuiteSupport>()
          .Append<components::Secdist>()
          .Append<components::DefaultSecdistProvider>()
          .Append<components::HttpClient>()
          .Append<clients::dns::Component>()
          .Append<server::handlers::TestsControl>();

  return utils::DaemonMain(argc, argv, components_list);
}
/// [Kafka service sample - main]
