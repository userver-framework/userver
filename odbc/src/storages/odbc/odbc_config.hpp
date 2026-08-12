#pragma once

#include <chrono>
#include <cstddef>
#include <optional>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

struct CommandControl {
    std::optional<std::chrono::milliseconds> network_timeout;
    std::optional<std::chrono::milliseconds> statement_timeout;
};

}  // namespace storages::odbc

USERVER_NAMESPACE_END
