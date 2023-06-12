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

#include <userver/clients/dns/config.hpp>
#include <userver/clients/dns/exception.hpp>
#include <userver/clients/dns/resolver.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/datetime.hpp>

#include <userver/utest/using_namespace_userver.hpp>

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

  logging::DefaultLoggerGuard guard{
      logging::MakeStderrLogger("default", logging::Format::kTskv,
                                logging::LevelFromString(config.log_level))};

  engine::RunStandalone(config.worker_threads, [&] {
    clients::dns::ResolverConfig resolver_config;
    resolver_config.network_timeout =
        std::chrono::milliseconds{config.timeout_ms};
    resolver_config.network_attempts = config.attempts;
    clients::dns::Resolver resolver{engine::current_task::GetTaskProcessor(),
                                    resolver_config};
    for (const auto& name : config.names) {
      try {
        auto response = resolver.Resolve(
            name, engine::Deadline::FromDuration(
                      std::chrono::milliseconds{config.timeout_ms}));
        std::cerr << "Got response for '" << name << "'\n";
        for (const auto& addr : response) {
          std::cerr << "  - " << addr.PrimaryAddressString() << '\n';
        }
      } catch (const std::exception& ex) {
        LOG_ERROR() << "Resolution failed: " << ex;
      }
    }
  });
}
