#pragma once

#include <chrono>
#include <optional>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/storages/scylla/session_config.hpp>
#include <userver/utils/default_dict.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla {

struct CommandControl final {
    std::optional<std::chrono::milliseconds> request_timeout;
    std::optional<Consistency> consistency;
    std::optional<SerialConsistency> serial_consistency;
};

CommandControl Parse(const formats::json::Value& value, formats::parse::To<CommandControl>);

extern const dynamic_config::Key<utils::DefaultDict<CommandControl>> kScyllaDefaultCommandControl;

}  // namespace storages::scylla

USERVER_NAMESPACE_END
