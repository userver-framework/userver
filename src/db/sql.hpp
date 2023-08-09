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

inline constexpr std::string_view kUpdateUser = R"~(
UPDATE real_medium.users SET 
  username = COALESCE($2, username),
  email = COALESCE($3, email),
  bio = COALESCE($4, bio),
  image = COALESCE($5, image),
  password_hash = COALESCE($6, password_hash)
WHERE user_id = $1
RETURNING *
)~";

inline constexpr std::string_view kFindUserById = R"~(
SELECT * FROM real_medium.users WHERE user_id = $1    
)~"; 

inline constexpr std::string_view kDeleteCommentById = R"~(
DELETE FROM real_medium.comments WHERE comment_id = $1     
)~"; 

inline constexpr std::string_view kAddComment = R"~(
INSERT INTO real_medium.comments(body, user_id, article_id) 
VALUES($1, $2, $3)
RETURNING *
)~"; 

inline constexpr std::string_view kFindIdArticleBySlug = R"~(
SELECT article_id FROM real_medium.articles WHERE slug = $1  
)~"; 

}