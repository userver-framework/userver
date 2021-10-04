#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include <boost/program_options.hpp>

#include <clients/dns/net_resolver.hpp>
#include <userver/clients/dns/exception.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/datetime.hpp>

namespace {

struct Config {
  std::string log_level = "error";
  size_t worker_threads = 1;
  int timeout_ms = 1000;
  int attempts = 1;
  std::vector<std::string> names;
};

Config ParseConfig(int argc, char** argv) {
  namespace po = boost::program_options;

  Config config;
  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "produce help message")(
      "log-level",
      po::value(&config.log_level)->default_value(config.log_level),
      "log level (trace, debug, info, warning, error)")(
      "worker-threads,t",
      po::value(&config.worker_threads)->default_value(config.worker_threads),
      "worker thread count")(
      "timeout",
      po::value(&config.timeout_ms)->default_value(config.timeout_ms),
      "network timeout (ms)")(
      "attempts", po::value(&config.attempts)->default_value(config.attempts),
      "network resolution attempts")("names", po::value(&config.names),
                                     "list of names to resolve");

  po::positional_options_description pos_desc;
  pos_desc.add("names", -1);

  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv)
                  .options(desc)
                  .positional(pos_desc)
                  .run(),
              vm);
    po::notify(vm);
  } catch (const std::exception& ex) {
    std::cerr << "Cannot parse command line: " << ex.what() << '\n';
    exit(1);
  }

  if (vm.count("help")) {
    std::cout << desc << '\n';
    exit(0);
  }

  return config;
}

}  // namespace

int main(int argc, char** argv) {
  auto config = ParseConfig(argc, argv);

  signal(SIGPIPE, SIG_IGN);

  engine::RunStandalone(config.worker_threads, [&] {
    logging::SetDefaultLoggerLevel(logging::LevelFromString(config.log_level));
    clients::dns::NetResolver::Config resolver_config;
    resolver_config.query_timeout =
        std::chrono::milliseconds{config.timeout_ms};
    resolver_config.attempts = config.attempts;
    clients::dns::NetResolver resolver{engine::current_task::GetTaskProcessor(),
                                       resolver_config};
    for (const auto& name : config.names) {
      try {
        auto response = resolver.Resolve(name).get();
        std::cerr << "Got response for '" << name
                  << "' with TTL=" << response.ttl.count() << "s at "
                  << utils::datetime::Timestring(response.received_at) << '\n';
        for (const auto& addr : response.addrs) {
          std::cerr << "  - " << addr.PrimaryAddressString() << '\n';
        }
      } catch (const clients::dns::NotResolvedException& ex) {
        LOG_ERROR() << "Resolution failed: " << ex;
      }
    }
  });
}
