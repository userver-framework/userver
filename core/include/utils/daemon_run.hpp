#pragma once

/// @file utils/daemon_run.hpp
/// @brief Functions to start a daemon with specified components list.

#include <components/component_list.hpp>

namespace utils {

/// Parses command line arguments and calls components::Run with config file
/// from --config parameter.
///
/// Other command line argumants:
/// * --init-log FILENAME - path to the initial log file, stdout if not set
/// * --help - show all command line argumants
int DaemonMain(int argc, char** argv,
               const components::ComponentList& components_list);

}  // namespace utils
