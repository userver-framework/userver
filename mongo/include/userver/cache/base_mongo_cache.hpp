#pragma once

/// @file userver/cache/base_mongo_cache.hpp
/// @brief @copybrief components::MongoCache

#include <chrono>

#include <fmt/format.h>

#include <userver/cache/cache_statistics.hpp>
#include <userver/cache/caching_component_base.hpp>
#include <userver/cache/mongo_cache_type_traits.hpp>
#include <userver/components/component_context.hpp>
#include <userver/formats/bson/document.hpp>
#include <userver/formats/bson/inline.hpp>
#include <userver/formats/bson/value_builder.hpp>
#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/operations.hpp>
#include <userver/storages/mongo/options.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/cpu_relax.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

inline const std::string kFetchAndParseStage = "fetch_and_parse";

inline constexpr std::chrono::milliseconds kCpuRelaxThreshold{10};
inline constexpr std::chrono::milliseconds kCpuRelaxInterval{2};

namespace impl {

std::chrono::milliseconds GetMongoCacheUpdateCorrection(const ComponentConfig&);

}

// clang-format off

/// @ingroup userver_components
///
/// @brief %Base class for all caches polling mongo collection
///
/// You have to provide a traits class in order to use this.
///
/// ## Static options:
/// All options of CachingComponentBase and
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// update-correction | adjusts incremental updates window to overlap with previous update | 0
///
/// ## Traits example:
/// All fields below (except for function overrides) are mandatory.
///
/// ```
/// struct MongoCacheTraitsExample {
///   // Component name for component
///   static constexpr auto kName = "mongo-dynamic-config";
///
///   // Collection to read from
///   static constexpr auto kMongoCollectionsField =
///       &storages::mongo::Collections::config;
///   // Update field name to use for incremental update (optional).
///   // When missing, incremental update is disabled.
///   // Please use reference here to avoid global variables
///   // initialization order issues.
///   static constexpr const std::string& kMongoUpdateFieldName =
///       mongo::db::taxi::config::kUpdated;
///
///   // Cache element type
///   using ObjectType = CachedObject;
///   // Cache element field name that is used as an index in the cache map
///   static constexpr auto kKeyField = &CachedObject::name;
///   // Type of kKeyField
///   using KeyType = std::string;
///   // Type of cache map, e.g. unordered_map, map, bimap
///   using DataType = std::unordered_map<KeyType, ObjectType>;
///
///   // Whether the cache prefers to read from replica (if true, you might get stale data)
///   static constexpr bool kIsSecondaryPreferred = true;
///
///   // Optional function that overrides BSON to ObjectType conversion
///   static constexpr auto DeserializeObject = &CachedObject::FromBson;
///   // or
///   static ObjectType DeserializeObject(const formats::bson::Document& doc) {
///     return doc["value"].As<ObjectType>();
///   }
///   // (default implementation calls doc.As<ObjectType>())
///   // For using default implementation
///   static constexpr bool kUseDefaultDeserializeObject = true;
///
///   // Optional function that overrides data retrieval operation
///   static storages::mongo::operations::Find GetFindOperation(
///      cache::UpdateType type,
///      const std::chrono::system_clock::time_point& last_update,
///      const std::chrono::system_clock::time_point& now,
///      const std::chrono::system_clock::duration& correction) {
///     mongo::operations::Find find_op({});
///     find_op.SetOption(mongo::options::Projection{"key", "value"});
///     return find_op;
///   }
///   // (default implementation queries kMongoUpdateFieldName: {$gt: last_update}
///   // for incremental updates, and {} for full updates)
///   // For using default implementation
///   static constexpr bool kUseDefaultFindOperation = true;
///
///   // Whether update part of the cache even if failed to parse some documents
///   static constexpr bool kAreInvalidDocumentsSkipped = false;
///
///   // Component to get the collections
///   using MongoCollectionsComponent = components::MongoCollections;
/// };
/// ```

// clang-format on

template <class MongoCacheTraits>
class MongoCache
    : public CachingComponentBase<typename MongoCacheTraits::DataType> {
  using CollectionsType = mongo_cache::impl::CollectionsType<
      decltype(MongoCacheTraits::kMongoCollectionsField)>;

 public:
  static constexpr std::string_view kName = MongoCacheTraits::kName;

  MongoCache(const ComponentConfig&, const ComponentContext&);

  ~MongoCache();

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  void Update(cache::UpdateType type,
              const std::chrono::system_clock::time_point& last_update,
              const std::chrono::system_clock::time_point& now,
              cache::UpdateStatisticsScope& stats_scope) override;

  typename MongoCacheTraits::ObjectType DeserializeObject(
      const formats::bson::Document& doc) const;

  storages::mongo::operations::Find GetFindOperation(
      cache::UpdateType type,
      const std::chrono::system_clock::time_point& last_update,
      const std::chrono::system_clock::time_point& now,
      const std::chrono::system_clock::duration& correction);

  std::unique_ptr<typename MongoCacheTraits::DataType> GetData(
      cache::UpdateType type);

  const std::shared_ptr<CollectionsType> mongo_collections_;
  const storages::mongo::Collection* const mongo_collection_;
  const std::chrono::system_clock::duration correction_;
  std::size_t cpu_relax_iterations_{0};
};

template <class MongoCacheTraits>
inline constexpr bool kHasValidate<MongoCache<MongoCacheTraits>> = true;

template <class MongoCacheTraits>
MongoCache<MongoCacheTraits>::MongoCache(const ComponentConfig& config,
                                         const ComponentContext& context)
    : CachingComponentBase<typename MongoCacheTraits::DataType>(config,
                                                                context),
      mongo_collections_(
          context
              .FindComponent<
                  typename MongoCacheTraits::MongoCollectionsComponent>()
              .template GetCollectionForLibrary<CollectionsType>()),
      mongo_collection_(std::addressof(
          mongo_collections_.get()->*MongoCacheTraits::kMongoCollectionsField)),
      correction_(impl::GetMongoCacheUpdateCorrection(config)) {
  [[maybe_unused]] mongo_cache::impl::CheckTraits<MongoCacheTraits>
      check_traits;

  if (CachingComponentBase<
          typename MongoCacheTraits::DataType>::GetAllowedUpdateTypes() ==
          cache::AllowedUpdateTypes::kFullAndIncremental &&
      !mongo_cache::impl::kHasUpdateFieldName<MongoCacheTraits> &&
      !mongo_cache::impl::kHasFindOperation<MongoCacheTraits>) {
    throw std::logic_error(
        "Incremental update support is requested in config but no update field "
        "name is specified in traits of '" +
        components::GetCurrentComponentName(config) + "' cache");
  }
  if (correction_.count() < 0) {
    throw std::logic_error(
        "Refusing to set forward (negative) update correction requested in "
        "config for '" +
        components::GetCurrentComponentName(config) + "' cache");
  }

  this->StartPeriodicUpdates();
}

template <class MongoCacheTraits>
MongoCache<MongoCacheTraits>::~MongoCache() {
  this->StopPeriodicUpdates();
}

template <class MongoCacheTraits>
void MongoCache<MongoCacheTraits>::Update(
    cache::UpdateType type,
    const std::chrono::system_clock::time_point& last_update,
    const std::chrono::system_clock::time_point& now,
    cache::UpdateStatisticsScope& stats_scope) {
  namespace sm = storages::mongo;

  const auto* collection = mongo_collection_;
  auto find_op = GetFindOperation(type, last_update, now, correction_);
  auto cursor = collection->Execute(find_op);
  if (type == cache::UpdateType::kIncremental && !cursor) {
    // Don't touch the cache at all
    LOG_INFO() << "No changes in cache " << MongoCacheTraits::kName;
    stats_scope.FinishNoChanges();
    return;
  }

  auto scope = tracing::Span::CurrentSpan().CreateScopeTime("copy_data");
  auto new_cache = GetData(type);

  // No good way to identify whether cursor accesses DB or reads buffed data
  scope.Reset(kFetchAndParseStage);

  utils::CpuRelax relax{cpu_relax_iterations_, &scope};
  std::size_t doc_count = 0;

  for (const auto& doc : cursor) {
    ++doc_count;

    relax.Relax();

    stats_scope.IncreaseDocumentsReadCount(1);

    try {
      auto object = DeserializeObject(doc);
      auto key = (object.*MongoCacheTraits::kKeyField);

      if (type == cache::UpdateType::kIncremental ||
          new_cache->count(key) == 0) {
        (*new_cache)[key] = std::move(object);
      } else {
        LOG_LIMITED_ERROR() << "Found duplicate key for 2 items in cache "
                            << MongoCacheTraits::kName << ", key=" << key;
      }
    } catch (const std::exception& e) {
      LOG_LIMITED_ERROR() << "Failed to deserialize cache item of cache "
                          << MongoCacheTraits::kName << ", _id="
                          << doc["_id"].template ConvertTo<std::string>()
                          << ", what(): " << e;
      stats_scope.IncreaseDocumentsParseFailures(1);

      if (!MongoCacheTraits::kAreInvalidDocumentsSkipped) throw;
    }
  }

  const auto elapsed_time = scope.ElapsedTotal(kFetchAndParseStage);
  if (elapsed_time > kCpuRelaxThreshold) {
    cpu_relax_iterations_ = static_cast<std::size_t>(
        static_cast<double>(doc_count) / (elapsed_time / kCpuRelaxInterval));
    LOG_TRACE() << fmt::format(
        "Elapsed time for updating {} {} for {} data items is over threshold. "
        "Will relax CPU every {} iterations",
        kName, elapsed_time.count(), doc_count, cpu_relax_iterations_);
  }

  scope.Reset();

  const auto size = new_cache->size();
  this->Set(std::move(new_cache));
  stats_scope.Finish(size);
}

template <class MongoCacheTraits>
typename MongoCacheTraits::ObjectType
MongoCache<MongoCacheTraits>::DeserializeObject(
    const formats::bson::Document& doc) const {
  if constexpr (mongo_cache::impl::kHasDeserializeObject<MongoCacheTraits>) {
    return MongoCacheTraits::DeserializeObject(doc);
  }
  if constexpr (mongo_cache::impl::kHasDefaultDeserializeObject<
                    MongoCacheTraits>) {
    return doc.As<typename MongoCacheTraits::ObjectType>();
  }
  UASSERT_MSG(false,
              "No deserialize operation defined but DeserializeObject invoked");
}

template <class MongoCacheTraits>
storages::mongo::operations::Find
MongoCache<MongoCacheTraits>::GetFindOperation(
    cache::UpdateType type,
    const std::chrono::system_clock::time_point& last_update,
    const std::chrono::system_clock::time_point& now,
    const std::chrono::system_clock::duration& correction) {
  namespace bson = formats::bson;
  namespace sm = storages::mongo;

  auto find_op = [&]() -> sm::operations::Find {
    if constexpr (mongo_cache::impl::kHasFindOperation<MongoCacheTraits>) {
      return MongoCacheTraits::GetFindOperation(type, last_update, now,
                                                correction);
    }
    if constexpr (mongo_cache::impl::kHasDefaultFindOperation<
                      MongoCacheTraits>) {
      bson::ValueBuilder query_builder(bson::ValueBuilder::Type::kObject);
      if constexpr (mongo_cache::impl::kHasUpdateFieldName<MongoCacheTraits>) {
        if (type == cache::UpdateType::kIncremental) {
          query_builder[MongoCacheTraits::kMongoUpdateFieldName] =
              bson::MakeDoc("$gt", last_update - correction);
        }
      }
      return sm::operations::Find(query_builder.ExtractValue());
    }
    UASSERT_MSG(false,
                "No find operation defined but GetFindOperation invoked");
  }();

  if (MongoCacheTraits::kIsSecondaryPreferred) {
    find_op.SetOption(sm::options::ReadPreference::kSecondaryPreferred);
  }
  return find_op;
}

template <class MongoCacheTraits>
std::unique_ptr<typename MongoCacheTraits::DataType>
MongoCache<MongoCacheTraits>::GetData(cache::UpdateType type) {
  if (type == cache::UpdateType::kIncremental) {
    auto ptr = this->Get();
    return std::make_unique<typename MongoCacheTraits::DataType>(*ptr);
  } else {
    return std::make_unique<typename MongoCacheTraits::DataType>();
  }
}

namespace impl {

std::string GetMongoCacheSchema();

}  // namespace impl

template <class MongoCacheTraits>
yaml_config::Schema MongoCache<MongoCacheTraits>::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<
      CachingComponentBase<typename MongoCacheTraits::DataType>>(
      impl::GetMongoCacheSchema());
}

}  // namespace components

USERVER_NAMESPACE_END
