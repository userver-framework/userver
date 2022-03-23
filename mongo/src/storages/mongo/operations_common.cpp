#include <storages/mongo/operations_common.hpp>

#include <chrono>
#include <limits>

#include <mongoc/mongoc.h>

#include <userver/storages/mongo/exception.hpp>

#include <formats/bson/wrappers.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl {

formats::bson::impl::BsonBuilder& EnsureBuilder(
    std::optional<formats::bson::impl::BsonBuilder>& optional_builder) {
  if (!optional_builder) {
    optional_builder.emplace();
  }
  return *optional_builder;
}

const bson_t* GetNative(
    const std::optional<formats::bson::impl::BsonBuilder>& builder) {
  return builder ? builder->Get() : nullptr;
}

cdriver::WriteConcernPtr MakeCDriverWriteConcern(
    options::WriteConcern::Level level) {
  cdriver::WriteConcernPtr write_concern(mongoc_write_concern_new());
  switch (level) {
    case options::WriteConcern::kMajority:
      mongoc_write_concern_set_wmajority(
          write_concern.get(),
          std::chrono::duration_cast<std::chrono::milliseconds>(
              options::WriteConcern::kDefaultMajorityTimeout)
              .count());
      break;
    case options::WriteConcern::kUnacknowledged:
      mongoc_write_concern_set_w(write_concern.get(), 0);
      break;
  }
  return write_concern;
}

cdriver::WriteConcernPtr MakeCDriverWriteConcern(
    const options::WriteConcern& wc_option) {
  if (wc_option.NodesCount() > std::numeric_limits<int32_t>::max()) {
    throw InvalidQueryArgumentException("Value ")
        << wc_option.NodesCount()
        << " of write concern nodes count is too high";
  }
  auto timeout_ms = wc_option.Timeout().count();

  cdriver::WriteConcernPtr write_concern(mongoc_write_concern_new());

  if (wc_option.IsMajority()) {
    mongoc_write_concern_set_wmajority(write_concern.get(), timeout_ms);
  } else {
    if (!wc_option.Tag().empty()) {
      mongoc_write_concern_set_wtag(write_concern.get(),
                                    wc_option.Tag().c_str());
    } else {
      mongoc_write_concern_set_w(write_concern.get(), wc_option.NodesCount());
    }
    mongoc_write_concern_set_wtimeout_int64(write_concern.get(), timeout_ms);
  }

  if (wc_option.Journal()) {
    mongoc_write_concern_set_journal(write_concern.get(), *wc_option.Journal());
  }

  return write_concern;
}

void AppendUpsert(formats::bson::impl::BsonBuilder& builder) {
  static const std::string kOptionName = "upsert";
  builder.Append(kOptionName, true);
}

}  // namespace storages::mongo::impl

USERVER_NAMESPACE_END
