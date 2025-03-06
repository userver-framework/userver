#pragma once

#include <string_view>

namespace functional_tests::db::sql {

inline constexpr std::string_view kCreateTable = R"~(
CREATE TABLE IF NOT EXISTS key_value_table (
key PRIMARY KEY,
value TEXT
)
)~";

inline constexpr std::string_view kSelectValueByKey = R"~(
SELECT value FROM key_value_table WHERE key = ?
)~";

inline constexpr std::string_view kSelectAllKeyValue = R"~(
SELECT * FROM key_value_table
)~";

inline constexpr std::string_view kInsertKeyValue = R"~(
INSERT OR IGNORE INTO key_value_table (key, value) VALUES (?, ?)
)~";

inline constexpr std::string_view kUpdateKeyValue = R"~(
UPDATE OR IGNORE key_value_table SET value = ? WHERE key = ?
)~";

inline constexpr std::string_view kDeleteKeyValue = R"~(
DELETE FROM key_value_table WHERE key = ?
)~";

}  // namespace functional_tests::db::sql
