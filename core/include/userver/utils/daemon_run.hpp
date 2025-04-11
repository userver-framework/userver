#pragma once

/// @file userver/utils/daemon_run.hpp
/// @brief Functions to start a daemon with specified components list.

#include <userver/components/component_list.hpp>
#include <userver/components/run.hpp>

namespace boost::program_options {
class options_description;
class variables_map;
}  // namespace boost::program_options

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @returns default options of DaemonMain
///
/// List of options:
/// * --help - show all command line arguments
/// * --config CONFIG - path to config.yaml
/// * --config_vars CONFIG_VARS - path to config_vars.yaml
/// * --config_vars_override CONFIG_VARS - path to config_vars.override.yaml
/// * --print-config-schema - print config.yaml YAML Schema
/// * --print-dynamic-config-defaults - print JSON with dynamic config defaults
boost::program_options::options_description BaseRunOptions();

/// Parses command line arguments and calls components::Run with config file
/// from --config parameter. See BaseRunOptions() for a list of options.
/// Reports unhandled exceptions.
int DaemonMain(int argc, const char* const argv[], const components::ComponentList& components_list);

/// Calls components::Run with config file from --config parameter.
/// Reports unhandled exceptions.
int DaemonMain(const boost::program_options::variables_map& vm, const components::ComponentList& components_list);

/// Calls components::Run with in-memory config.
/// Reports unhandled exceptions.
int DaemonMain(const components::InMemoryConfig& config, const components::ComponentList& components_list);

}  // namespace utils

USERVER_NAMESPACE_END
