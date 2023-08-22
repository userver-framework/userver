#pragma once
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <userver/cache/base_postgres_cache.hpp>
#include <userver/storages/postgres/io/chrono.hpp>

#include "models/comment.hpp"
#include "models/article.hpp"

namespace real_medium::cache::comments_cache {

class CommentsCacheContainer
{
 public:
  using Key=real_medium::models::ArticleId;
  using Comment=real_medium::models::CachedComment;
  using CommentPtr=std::shared_ptr<const Comment>;
  void insert_or_assign(Key &&key, Comment &&config);
  size_t size() const;

  std::unordered_set<CommentPtr> findComments(const Key &key) const;

 private:
  std::unordered_map<Key, std::unordered_set<CommentPtr>> comments_to_key_;
};

struct CommentCachePolicy {
  static constexpr auto kName = "comments-cache";
  using ValueType = real_medium::models::CachedComment;
  using CacheContainer = CommentsCacheContainer;
  static constexpr auto kKeyMember = &real_medium::models::CachedComment::id;
  static userver::storages::postgres::Query kQuery;
  static constexpr auto kUpdatedField = "Comments::updated_at";
  using UpdatedFieldType = userver::storages::postgres::TimePointTz;
};

using CommentsCache = ::userver::components::PostgreCache<CommentCachePolicy>;
}