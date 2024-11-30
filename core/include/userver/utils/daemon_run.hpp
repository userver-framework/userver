#pragma once

/// @file userver/utils/daemon_run.hpp
/// @brief Functions to start a daemon with specified components list.

#include <userver/components/component_list.hpp>
#include <userver/components/run.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// Parses command line arguments and calls components::Run with config file
/// from --config parameter. Reports unhandled exceptions.
///
/// Other command line arguments:
/// * --help - show all command line arguments
/// * --config CONFIG - path to config.yaml
/// * --config_vars CONFIG_VARS - path to config_vars.yaml
/// * --config_vars_override CONFIG_VARS - path to config_vars.override.yaml
/// * --print-config-schema - print config.yaml YAML Schema
/// * --print-dynamic-config-defaults - print JSON with dynamic config defaults
int DaemonMain(int argc, const char* const argv[], const components::ComponentList& components_list);

/// Calls components::Run with in-memory config.
/// Reports unhandled exceptions.
int DaemonMain(const components::InMemoryConfig& config, const components::ComponentList& components_list);

}  // namespace utils

USERVER_NAMESPACE_END
