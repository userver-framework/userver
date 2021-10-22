#include <iostream>

#include <boost/program_options.hpp>

#include <userver/congestion_control/controller.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/logging/log.hpp>

#include <userver/utest/using_namespace_userver.hpp>

using namespace congestion_control;

struct Config {
  Policy policy;
  std::string log_level = "none";
};

Config ParseArgs(int argc, char* argv[]) {
  Config config;
  std::string policy_json;

  namespace po = boost::program_options;

  // clang-format off
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help,h", "produce help message")
    ("log-level",
      po::value(&config.log_level)->default_value(config.log_level),
      "log level (trace, debug, info, warning, error)")
    ("policy,p",
     po::value(&policy_json)->default_value(std::string{}),
     "policy in JSON")
  ;
  // clang-format on

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    exit(0);
  }

  if (!policy_json.empty()) {
    config.policy = MakePolicy(formats::json::FromString(policy_json));
  }

  return config;
}

int main(int argc, char* argv[]) {
  Config config = ParseArgs(argc, argv);

  logging::SetDefaultLoggerLevel(logging::LevelFromString(config.log_level));

  Controller ctrl("cc", config.policy);

  for (;;) {
    Sensor::Data data;
    std::string line;
    std::getline(std::cin, line);
    if (line.empty()) break;

    auto rc = std::sscanf(line.data(), "%lu %lu", &data.current_load,
                          &data.overload_events_count);
    if (rc != 2) throw std::runtime_error("Invalid line: " + line);

    ctrl.Feed(data);
    auto limit = ctrl.GetLimit();
    if (limit.load_limit) {
      std::cout << *limit.load_limit << std::endl;
    } else {
      std::cout << "(none)" << std::endl;
    }
  }
}
