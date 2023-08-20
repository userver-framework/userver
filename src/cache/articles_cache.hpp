#pragma once
#include <memory>
#include <unordered_map>
#include <userver/cache/base_postgres_cache.hpp>
#include <userver/storages/postgres/io/chrono.hpp>

#include "../models/article.hpp"

namespace real_medium::cache::articles_cache {
class ArticlesCacheContainer
{
 public:
  using Key=real_medium::models::ArticleId;
  using Article=real_medium::models::TaggedArticleWithProfile;
  using ArticlePtr=std::shared_ptr<Article>;
  using AuthorName=std::string;
  void insert_or_assign(Key &&key, Article &&config);
  size_t size() const;

  ArticlePtr FindConfig(const Key &key) const;
  //std::vector<ArticlePtr> FindConfigsByService(std::string_view service) const;
  //std::vector<ArticlePtr> FindConfigs(std::string_view service,
  //                                   const std::vector<std::string> &ids) const;

 private:
  std::unordered_map<Key, ArticlePtr> articles_to_key_;
  std::unordered_map<AuthorName, std::vector<ArticlePtr>> articles_by_author_;
};



}
