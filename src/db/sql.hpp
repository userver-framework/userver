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
inline constexpr std::string_view kFindCommentByIdAndSlug = R"~(
WITH article AS (
  SELECT article_id FROM real_medium.articles WHERE slug = $2
)
SELECT * FROM real_medium.comments 
JOIN article ON article.article_id = real_medium.comments.article_id
WHERE comment_id = $1
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
SET favorites_count=favorites_count - 1
WHERE article_id=$1
)~";

inline constexpr std::string_view kFindArticlesByFollowedUsers = R"~(
SELECT real_medium.get_feed_articles($1, $2, $3)
)~";

inline constexpr std::string_view kGetArticleIdBySlug{R"~(
SELECT real_medium.get_article_id_by_slug($1)
)~"};

inline constexpr std::string_view kUpdateArticleBySlug{R"~(
SELECT real_medium.update_article_by_slug($1, $2, $3, $4, $5, $6)
)~"};

inline constexpr std::string_view kDeleteArticleBySlug{R"~(
SELECT real_medium.delete_article_by_slug($1, $2)
)~"};

inline constexpr std::string_view kFindArticlesByFilters{R"~(
SELECT real_medium.get_articles_by_filters($1, $2, $3, $4, $5, $6)
)~"};


inline constexpr std::string_view kSelectFullArticleInfo= {R"~(
SELECT a.article_id AS articleId,
       a.title,
       a.slug,
       a.description,
       a.body,
       ARRAY(
              SELECT tag_name
              FROM real_medium.tag_list tl
              JOIN real_medium.article_tag at ON tl.tag_id = at.tag_id
              WHERE at.article_id = a.article_id
       ) AS tags,
       a.created_at AS createdAt,
       a.updated_at AS updatedAt,
       ARRAY(
              SELECT user_id
              FROM real_medium.favorites f
              WHERE f.article_id = a.article_id
       ) AS article_favorited_by_user_ids,

       ARRAY(
              SELECT u.username
              FROM real_medium.favorites f
              JOIN real_medium.users u ON f.user_id = u.user_id
              WHERE f.article_id = a.article_id
       ) AS article_favorited_by_usernames,
       ARRAY(
              SELECT follower_user_id
              FROM real_medium.followers fl
              WHERE fl.followed_user_id = a.user_id
       ) AS author_followed_by_user_ids,
       ROW(u.user_id, u.username, u.email, u.bio, u.image, u.password_hash)::real_medium.user AS author_info
FROM real_medium.articles a
JOIN real_medium.users u ON a.user_id = u.user_id
)~"
};

}  // namespace real_medium::sql
