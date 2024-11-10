#pragma once

/// @file userver/storages/sqlite/options.hpp
/// @brief Options

#include <optional>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

struct TransactionOptions {};

struct CommandControl {};

using OptionalCommandControl = std::optional<CommandControl>;

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
