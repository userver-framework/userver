#pragma once

/// @file userver/utils/daemon_run.hpp
/// @brief Functions to start a daemon with specified components list.

#include <userver/components/component_list.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// Parses command line arguments and calls components::Run with config file
/// from --config parameter.
///
/// Other command line argumants:
/// * --init-log FILENAME - path to the initial log file, stdout if not set
/// * --help - show all command line argumants
/// * --config CONFIG - path to config.yaml
/// * --config_vars CONFIG_VARS - path to config_vars.yaml
/// * --config_vars_override CONFIG_VARS - path to config_vars.override.yaml
int DaemonMain(int argc, const char* const argv[],
               const components::ComponentList& components_list);

}  // namespace utils

USERVER_NAMESPACE_END
