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

template <class Value>
std::optional<std::string> ToOptional(const Value& val) {
    if (val.empty())
        return {};
    else
        return {val.template as<std::string>()};
}

}  // namespace

boost::program_options::options_description BaseRunOptions() {
    namespace po = boost::program_options;
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()
        ("help,h", "produce this help message")
        ("print-config-schema", "print config.yaml YAML Schema")
        ("print-dynamic-config-defaults", "print JSON object with dynamic config defaults")
        ("config_vars", po::value<std::string>(), "path to config_vars.yaml; if set, config_vars in config.yaml are ignored")
        ("config_vars_override", po::value<std::string>(), "path to an additional config_vars.yaml, which overrides vars of config_vars.yaml")
    ;
    // clang-format on
    return desc;
}

int DaemonMain(const int argc, const char* const argv[], const components::ComponentList& components_list) {
    namespace po = boost::program_options;
    po::variables_map vm;
    auto desc = BaseRunOptions();
    desc.add_options()("config,c", po::value<std::string>()->required(), "path to server config");

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

    return DaemonMain(vm, components_list);
}

int DaemonMain(const boost::program_options::variables_map& vm, const components::ComponentList& components_list) {
    utils::impl::FinishStaticRegistration();

    if (vm.count("print-config-schema")) {
        std::cout << components::impl::GetStaticConfigSchema(components_list) << "\n";
        return 0;
    }

    if (vm.count("print-dynamic-config-defaults")) {
        std::cout << components::impl::GetDynamicConfigDefaults() << "\n";
        return 0;
    }

    try {
        components::Run(
            vm["config"].as<std::string>(),
            ToOptional(vm["config_vars"]),
            ToOptional(vm["config_vars_override"]),
            components_list
        );
        return 0;
    } catch (const std::exception& ex) {
        auto msg = fmt::format("Unhandled exception in components::Run: {}", ex.what());
        std::cerr << msg << "\n";
        return 1;
    } catch (...) {
        auto msg = fmt::format(
            "Non-standard exception in components::Run: {}", boost::current_exception_diagnostic_information()
        );
        std::cerr << msg << '\n';
        return 1;
    }
}

int DaemonMain(const components::InMemoryConfig& config, const components::ComponentList& components_list) {
    utils::impl::FinishStaticRegistration();

    try {
        components::Run(config, components_list);
        return 0;
    } catch (const std::exception& ex) {
        auto msg = fmt::format("Unhandled exception in components::Run: {}", ex.what());
        std::cerr << msg << "\n";
        return 1;
    } catch (...) {
        auto msg = fmt::format(
            "Non-standard exception in components::Run: {}", boost::current_exception_diagnostic_information()
        );
        std::cerr << msg << '\n';
        return 1;
    }
}

}  // namespace utils

USERVER_NAMESPACE_END
