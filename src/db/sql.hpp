#pragma once

#include <string_view>

namespace real_medium::sql {

inline constexpr std::string_view kInsertUser = R"~(
INSERT INTO real_medium.users(username, email, password_hash) 
VALUES($1, $2, $3)
RETURNING *
)~";

inline constexpr std::string_view kSelectUserByEmailAndPassword = R"~(
SELECT * FROM real_medium.users
WHERE email = $1 AND password_hash = $2
)~";

}