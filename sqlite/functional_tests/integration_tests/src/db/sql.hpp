#pragma once

#include <userver/utils/string_literal.hpp>

namespace functional_tests::db::sql {

inline constexpr utils::StringLiteral kCreateTable = R"~(
CREATE TABLE IF NOT EXISTS key_value_table (
key PRIMARY KEY,
value TEXT
)
)~";

inline constexpr utils::StringLiteral kSelectValueByKey = R"~(
SELECT value FROM key_value_table WHERE key = ?
)~";

inline constexpr utils::StringLiteral kSelectAllKeyValue = R"~(
SELECT * FROM key_value_table
)~";

inline constexpr utils::StringLiteral kInsertKeyValue = R"~(
INSERT OR IGNORE INTO key_value_table (key, value) VALUES (?, ?)
)~";

inline constexpr utils::StringLiteral kUpdateKeyValue = R"~(
UPDATE OR IGNORE key_value_table SET value = ? WHERE key = ?
)~";

inline constexpr utils::StringLiteral kDeleteKeyValue = R"~(
DELETE FROM key_value_table WHERE key = ?
)~";

}  // namespace functional_tests::db::sql
