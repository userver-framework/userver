#include "articles_cache.hpp"

namespace real_medium::cache::articles_cache {

userver::storages::postgres::Query ArticlesCachePolicy::kQuery =
    userver::storages::postgres::Query(
        real_medium::sql::kSelectFullArticleInfo.data());

void ArticlesCacheContainer::insert_or_assign(Key &&key, Article &&article) {
  auto articlePtr = std::make_shared<const Article>(std::move(article));
  auto oldValue=articleByKey_.find(key);
  if(oldValue!=articleByKey_.end())
    articleBySlug_.erase(oldValue->second->slug);

  articleByKey_.insert_or_assign(key,articlePtr);
  articleBySlug_.insert_or_assign(articlePtr->slug,articlePtr);
  articlesByAuthor_[articlePtr->authorInfo.id].insert_or_assign(articlePtr->createdAt,articlePtr);
  recentArticles_.insert_or_assign(articlePtr->createdAt,articlePtr);
}

size_t ArticlesCacheContainer::size() const { return articleByKey_.size(); }

ArticlesCacheContainer::ArticlePtr ArticlesCacheContainer::findArticle(const Key &key) const{
  auto it=articleByKey_.find(key);
  if(it==articleByKey_.end())
    return nullptr;
  return it->second;
}

ArticlesCacheContainer::ArticlePtr ArticlesCacheContainer::findArticleBySlug(const Slug& slug) const{
  auto it=articleBySlug_.find(slug);
  if(it==articleBySlug_.end())
    return nullptr;
  return it->second;
}

std::vector<ArticlesCacheContainer::ArticlePtr> ArticlesCacheContainer::getRecent(real_medium::dto::ArticleFilterDTO &filter) const{
  std::vector<ArticlePtr> articles;
  int offset=0;
  for(const auto &it:recentArticles_){
    if(filter.limit && articles.size()>=filter.limit)
      break;

    const auto &tags=it.second->tags;
    if(filter.tag && it.second->tags.find(filter.tag.value())==tags.end())
      continue;

    if(filter.author && it.second->authorInfo.username!=filter.author)
      continue;

    const auto &favorited=it.second->articleFavoritedByUsernames;
    if(filter.favorited && favorited.find(filter.favorited.value())==favorited.end())
      continue;

    if(filter.offset && offset<filter.offset){
      ++offset;
      continue;
    }
    articles.push_back(it.second);
  }
  return articles;
}
std::vector<ArticlesCacheContainer::ArticlePtr> ArticlesCacheContainer::getFeed(real_medium::dto::FeedArticleFilterDTO &filter, UserId authId) const{
  std::vector<ArticlePtr> articles;
  auto followedArticles=articlesByAuthor_.find(authId);
  if(followedArticles==articlesByAuthor_.end())
    return articles;
  int offset=0;
  for(const auto &it:followedArticles->second){
    if(filter.limit && articles.size()>=filter.limit)
      break;
    if(filter.offset && offset<filter.offset){
      ++offset;
      continue;
    }
    articles.push_back(it.second);
  }
  return articles;
}

}
