#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include <boost/program_options.hpp>

#include <userver/clients/dns/resolver.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/logger.hpp>

#include <userver/static_config/dns_client.hpp>

#include <userver/utest/using_namespace_userver.hpp>

namespace {

struct Config {
    std::string log_level = "error";
    size_t worker_threads = 1;
    int timeout_ms = 1000;
    int attempts = 1;
    std::vector<std::string> names;
};

Config ParseConfig(int argc, const char* const argv[]) {
    namespace po = boost::program_options;

    Config c;
    po::options_description desc("Allowed options");

    // clang-format off
    desc.add_options()
      ("help,h", "produce help message")
      ("log-level", po::value(&c.log_level)->default_value(c.log_level), "log level (trace, debug, info, warning, error)")
      ("worker-threads,t", po::value(&c.worker_threads)->default_value(c.worker_threads), "worker thread count")
      ("timeout", po::value(&c.timeout_ms)->default_value(c.timeout_ms), "network timeout (ms)")
      ("attempts", po::value(&c.attempts)->default_value(c.attempts), "network resolution attempts")
      ("names", po::value(&c.names), "list of names to resolve")
    ;
    // clang-format on

    po::positional_options_description pos_desc;
    pos_desc.add("names", -1);

    po::variables_map vm;
    try {
        po::store(po::command_line_parser(argc, argv).options(desc).positional(pos_desc).run(), vm);
        po::notify(vm);
    } catch (const std::exception& ex) {
        std::cerr << "Cannot parse command line: " << ex.what() << '\n';
        std::exit(1);  // NOLINT(concurrency-mt-unsafe)
    }

    if (vm.count("help")) {
        std::cout << desc << '\n';
        std::exit(0);  // NOLINT(concurrency-mt-unsafe)
    }

    return c;
}

}  // namespace

int main(int argc, const char* const argv[]) {
    const auto config = ParseConfig(argc, argv);

    signal(SIGPIPE, SIG_IGN);

    const logging::DefaultLoggerGuard guard{
        logging::MakeStderrLogger("default", logging::Format::kTskv, logging::LevelFromString(config.log_level))};

    engine::RunStandalone(config.worker_threads, [&] {
        ::userver::static_config::DnsClient resolver_config;
        resolver_config.network_timeout = std::chrono::milliseconds{config.timeout_ms};
        resolver_config.network_attempts = config.attempts;
        clients::dns::Resolver resolver{engine::current_task::GetBlockingTaskProcessor(), resolver_config};
        for (const auto& name : config.names) {
            try {
                auto response = resolver.Resolve(
                    name, engine::Deadline::FromDuration(std::chrono::milliseconds{config.timeout_ms})
                );
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
