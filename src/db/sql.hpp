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

inline constexpr std::string_view kFindUserIDByUsername = R"~(
SELECT user_id FROM real_medium.users WHERE username = $1    
)~";

// Comments
inline constexpr std::string_view kFindCommentById = R"~(
SELECT * FROM real_medium.comments WHERE comment_id = $1 
)~";

inline constexpr std::string_view kFindCommentsByArticleId = R"~(
WITH comments AS (
  SELECT * FROM real_medium.comments WHERE article_id = $1
)
SELECT 
    comments.comment_id, 
    comments.created_at, 
    comments.updated_at,
    comments.body,
    (
        SELECT 
            ROW(users.username, users.bio, users.image,
            CASE WHEN EXISTS (SELECT 1 FROM real_medium.followers 
            WHERE followers.followed_user_id = comments.user_id AND followers.follower_user_id = $2)
            THEN true ELSE false END)::real_medium.profile
        FROM real_medium.users
        WHERE user_id = comments.user_id
    ) AS author
FROM comments
)~";

inline constexpr std::string_view kDeleteCommentById = R"~(
DELETE FROM real_medium.comments WHERE comment_id = $1 AND user_id = $2
RETURNING *
)~";

inline constexpr std::string_view kAddComment = R"~(
WITH comment AS (
    INSERT INTO real_medium.comments(body, user_id, article_id) 
    VALUES ($1, $2, $3)
    RETURNING *
)
SELECT 
    comment.comment_id, 
    comment.created_at, 
    comment.updated_at,
    comment.body,
    (
        SELECT 
            ROW(users.username, users.bio, users.image, false)::real_medium.profile
        FROM real_medium.users
        WHERE user_id = $2
    ) AS author
FROM comment
)~";

inline constexpr std::string_view kFindIdArticleBySlug = R"~(
SELECT article_id FROM real_medium.articles WHERE slug = $1  
)~";

inline constexpr std::string_view kIsProfileFollowing = R"~(
RETURN EXISTS (SELECT 1 FROM real_medium.followers WHERE follower_user_id = $1 AND followed_user_id = $2);
)~";

// TODO: reuse common kIsProfileFollowing
inline constexpr std::string_view kGetProfileByUsername = R"~(
WITH profile AS (
  SELECT * FROM real_medium.users WHERE username = $1
)
SELECT profile.username, profile.bio, profile.image, 
       CASE WHEN EXISTS (
         SELECT 1 FROM real_medium.followers 
         WHERE followed_user_id = profile.user_id AND follower_user_id = $2
       ) THEN true ELSE false END AS following
FROM profile
)~";

inline constexpr std::string_view KFollowingUser = R"~(
WITH profile AS (
  SELECT * FROM real_medium.users WHERE user_id = $1
), following AS (
  INSERT INTO real_medium.followers(followed_user_id, follower_user_id) VALUES ($1, $2)
  ON CONFLICT DO NOTHING
  RETURNING *
)
SELECT 
  profile.username, 
  profile.bio, 
  profile.image, 
  CASE WHEN EXISTS (SELECT 1 FROM following) THEN TRUE ELSE FALSE END
FROM profile
)~";

inline constexpr std::string_view KUnFollowingUser = R"~(
WITH profile AS (
  SELECT * FROM real_medium.users WHERE user_id = $1
), following AS (
  DELETE FROM real_medium.followers WHERE followed_user_id = $1 AND follower_user_id = $2
  RETURNING *
)
SELECT 
  profile.username, 
  profile.bio, 
  profile.image, 
  CASE WHEN EXISTS (SELECT 1 FROM following) THEN FALSE ELSE TRUE END
FROM profile
)~";

inline constexpr std::string_view kCreateArticle{R"~(
SELECT real_medium.create_article($1, $2, $3, $4, $5, $6)
)~"};

inline constexpr std::string_view kGetArticleWithAuthorProfile{R"~(
SELECT real_medium.get_article_with_author_profile($1, $2)
)~"};

inline constexpr std::string_view kGetArticleWithAuthorProfileBySlug{R"~(
SELECT real_medium.get_article_with_author_profile_by_slug($1, $2)
)~"};

inline constexpr std::string_view kInsertFavoritePair = R"~(
WITH tmp(article_id, user_id) AS (
    SELECT article_id, $1 FROM real_medium.articles WHERE slug=$2
)
INSERT INTO real_medium.favorites(article_id, user_id) (SELECT article_id, user_id FROM tmp)
ON CONFLICT DO NOTHING
RETURNING article_id
)~";

inline constexpr std::string_view kIncrementFavoritesCount = R"~(
UPDATE real_medium.articles
SET favorites_count=favorites_count + 1
WHERE article_id=$1
)~";

inline constexpr std::string_view kDeleteFavoritePair = R"~(
WITH tmp(article_id, user_id) AS (
    SELECT article_id, $1 FROM real_medium.articles WHERE slug=$2
)
DELETE FROM real_medium.favorites
WHERE (article_id, user_id) IN (SELECT article_id, user_id FROM tmp)
RETURNING article_id
)~";

inline constexpr std::string_view kDecrementFavoritesCount = R"~(
UPDATE real_medium.articles
zSET favorites_count=favorites_count - 1
WHERE article_id=$1
)~";

inline constexpr std::string_view kFindArticlesByFollowedUsers = R"~(
SELECT real_medium.get_feed_articles($1, $2, $3)
)~";
}  // namespace real_medium::sql
