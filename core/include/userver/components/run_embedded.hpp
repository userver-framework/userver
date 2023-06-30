#pragma once

#include <mutex>

#include <userver/components/run.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

/// Starts a server with the provided component list and config.
/// Ropens the logging files on SIGUSR1.
///
/// @see utils::DaemonMain
void RunEmbedded(std::mutex& stopper_mutex,
		 const InMemoryConfig& config,
		 const ComponentList& component_list,
                 const std::string& init_log_path = {},
                 logging::Format format = logging::Format::kTskv);

}  // namespace components

USERVER_NAMESPACE_END
