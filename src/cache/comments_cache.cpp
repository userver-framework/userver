#include "userver/storages/postgres/query.hpp"
#include <userver/cache/base_postgres_cache.hpp>
#include <userver/storages/postgres/io/chrono.hpp>
#include "userver/utils/algo.hpp"

#include "comments_cache.hpp"
#include "db/sql.hpp"

namespace real_medium::cache::comments_cache {

userver::storages::postgres::Query CommentCachePolicy::kQuery =
    userver::storages::postgres::Query(
        real_medium::sql::kSelectCachedComments.data());

void CommentsCacheContainer::insert_or_assign(real_medium::cache::comments_cache::CommentsCacheContainer::Key&& key,
      real_medium::cache::comments_cache::CommentsCacheContainer::Comment&& comment) {
      auto commentPtr = std::make_shared<const Comment>(std::move(comment));
      comments_to_key_[key].insert(commentPtr);
};

size_t CommentsCacheContainer::size() const {
  return comments_to_key_.size();
}

std::unordered_set<CommentsCacheContainer::CommentPtr> CommentsCacheContainer::findComments(const real_medium::cache::comments_cache::CommentsCacheContainer::Key& key) const {
  if (!comments_to_key_.count(key))
    return {};
  return comments_to_key_.at(key);
}

}
