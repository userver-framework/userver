#include <userver/utils/daemon_run.hpp>

#include <iostream>

#include <boost/program_options.hpp>

#include <userver/components/run.hpp>

#include <boost/exception/diagnostic_information.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace {

std::optional<std::string> ToOptional(std::string&& s) {
  if (s.empty())
    return {};
  else
    return {std::move(s)};
}

}  // namespace

int DaemonMain(int argc, char** argv,
               const components::ComponentList& components_list) {
  namespace po = boost::program_options;

  po::variables_map vm;
  po::options_description desc("Allowed options");
  std::string config_path = "config_dev.yaml";
  std::string config_vars_path;
  std::string config_vars_override_path;
  std::string init_log_path;

  // clang-format off
  desc.add_options()
    ("help,h", "produce this help message")
    ("config,c", po::value(&config_path)->default_value(config_path), "path to server config")
    ("config_vars", po::value(&config_vars_path), "path to config_vars.yaml; if set, config_vars in config.yaml are ignored")
    ("config_vars_override", po::value(&config_vars_override_path), "path to an additional config_vars.yaml, which overrides vars of config_vars.yaml")
    ("init-log,l", po::value(&init_log_path), "path to initialization log")
  ;
  // clang-format on

  try {
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
  } catch (const std::exception& ex) {
    std::cerr << ex.what() << '\n';
    return 1;
  }

  if (vm.count("help")) {
    std::cerr << desc << '\n';
    return 0;
  }

  try {
    components::Run(config_path, ToOptional(std::move(config_vars_path)),
                    ToOptional(std::move(config_vars_override_path)),
                    components_list, init_log_path);
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << "Unhandled exception in components::Run: " << ex.what()
              << '\n';
    return 1;
  } catch (...) {
    std::cerr << "Non-standard exception in components::Run: "
              << boost::current_exception_diagnostic_information() << '\n';
    return 1;
  }
}

}  // namespace utils

USERVER_NAMESPACE_END
