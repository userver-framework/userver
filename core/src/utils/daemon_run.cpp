#include <userver/utils/daemon_run.hpp>

#include <iostream>

#include <boost/exception/diagnostic_information.hpp>
#include <boost/program_options.hpp>

#include <userver/components/run.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/impl/static_registration.hpp>

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

int DaemonMain(const int argc, const char* const argv[],
               const components::ComponentList& components_list) {
  utils::impl::FinishStaticRegistration();

  namespace po = boost::program_options;

  po::variables_map vm;
  po::options_description desc("Allowed options");
  std::string config_path = "config_dev.yaml";
  std::string config_vars_path;
  std::string config_vars_override_path;

  // clang-format off
  desc.add_options()
    ("help,h", "produce this help message")
    ("print-config-schema", "print config.yaml YAML Schema")
    ("print-dynamic-config-defaults", "print JSON object with dynamic config defaults")
    ("config,c", po::value(&config_path)->default_value(config_path), "path to server config")
    ("config_vars", po::value(&config_vars_path), "path to config_vars.yaml; if set, config_vars in config.yaml are ignored")
    ("config_vars_override", po::value(&config_vars_override_path), "path to an additional config_vars.yaml, which overrides vars of config_vars.yaml")
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

  if (vm.count("print-config-schema")) {
    std::cout << components::impl::GetStaticConfigSchema(components_list)
              << "\n";
    return 0;
  }

  if (vm.count("print-dynamic-config-defaults")) {
    std::cout << components::impl::GetDynamicConfigDefaults() << "\n";
    return 0;
  }

  try {
    components::Run(config_path, ToOptional(std::move(config_vars_path)),
                    ToOptional(std::move(config_vars_override_path)),
                    components_list);
    return 0;
  } catch (const std::exception& ex) {
    auto msg =
        fmt::format("Unhandled exception in components::Run: {}", ex.what());
    std::cerr << msg << "\n";
    return 1;
  } catch (...) {
    auto msg = fmt::format("Non-standard exception in components::Run: {}",
                           boost::current_exception_diagnostic_information());
    std::cerr << msg << '\n';
    return 1;
  }
}

}  // namespace utils

USERVER_NAMESPACE_END
