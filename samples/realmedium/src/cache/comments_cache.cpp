#include <userver/cache/base_postgres_cache.hpp>
#include <userver/storages/postgres/io/chrono.hpp>
#include "userver/storages/postgres/query.hpp"
#include "userver/utils/algo.hpp"

#include "comments_cache.hpp"
#include "db/sql.hpp"

namespace real_medium::cache::comments_cache {

userver::storages::postgres::Query CommentCachePolicy::kQuery =
    userver::storages::postgres::Query(
        real_medium::sql::kSelectCachedComments.data());

void CommentsCacheContainer::insert_or_assign(
    real_medium::cache::comments_cache::CommentsCacheContainer::Key&& commentId,
    real_medium::cache::comments_cache::CommentsCacheContainer::Comment&&
        comment) {
  auto commentPtr = std::make_shared<const Comment>(std::move(comment));
  if (comment_to_key_.count(commentId)) {
    auto& oldSlug = comment_to_key_[commentId]->slug;
    if (oldSlug != commentPtr->slug) {
      auto& comments = comments_to_slug_[oldSlug];
      comments_to_slug_[commentPtr->slug] = comments;
      comments_to_slug_.erase(oldSlug);
    }
  }
  comment_to_key_.insert_or_assign(commentId, commentPtr);
  comments_to_slug_[commentPtr->slug].insert_or_assign(commentId, commentPtr);
};

size_t CommentsCacheContainer::size() const { return comment_to_key_.size(); }

std::map<CommentsCacheContainer::Key, CommentsCacheContainer::CommentPtr>
CommentsCacheContainer::findComments(const Slug& slug) const {
  if (!comments_to_slug_.count(slug)) return {};
  return comments_to_slug_.at(slug);
}

}  // namespace real_medium::cache::comments_cache
