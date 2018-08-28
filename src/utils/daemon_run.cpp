#include <utils/daemon_run.hpp>

#include <iostream>

#include <boost/program_options.hpp>

#include <components/run.hpp>

namespace utils {

int DaemonMain(int argc, char** argv,
               const components::ComponentList& components_list) {
  namespace po = boost::program_options;

  po::variables_map vm;
  po::options_description desc("Allowed options");
  std::string config_path = "config_dev.json";
  std::string init_log_path;

  // clang-format off
  desc.add_options()
    ("help,h", "produce this help message")
    ("config,c", po::value(&config_path)->default_value(config_path), "path to server config")
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
    components::Run(config_path, components_list, init_log_path);
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << "Unhandled exception: " << ex.what() << '\n';
    return 1;
  }
}

}  // namespace utils
