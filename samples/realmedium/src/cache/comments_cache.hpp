#pragma once
#include <memory>
#include <map>
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
  using Key=real_medium::models::CommentId;
  using Comment=real_medium::models::CachedComment;
  using CommentPtr=std::shared_ptr<const Comment>;
  using Slug=std::string;
  void insert_or_assign(Key &&key, Comment &&config);
  size_t size() const;

  std::map<Key, CommentPtr> findComments(const Slug &key) const;

 private:
  std::unordered_map<Slug, std::map<Key, CommentPtr>> comments_to_slug_;
  std::unordered_map<Key, CommentPtr> comment_to_key_;
};

struct CommentCachePolicy {
  static constexpr auto kName = "comments-cache";
  using ValueType = real_medium::models::CachedComment;
  using CacheContainer = CommentsCacheContainer;
  static constexpr auto kKeyMember = &real_medium::models::CachedComment::id;
  static userver::storages::postgres::Query kQuery;
  static constexpr auto kUpdatedField = "c.updated_at";
  using UpdatedFieldType = userver::storages::postgres::TimePointTz;
};

using CommentsCache = ::userver::components::PostgreCache<CommentCachePolicy>;
}