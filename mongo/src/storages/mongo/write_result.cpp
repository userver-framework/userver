#include <userver/storages/mongo/write_result.hpp>

#include <string>

#include <bson/bson.h>
#include <mongoc/mongoc.h>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo {
namespace {

const std::string kFamStatus = "lastErrorObject";
const std::string kFamStatusUpdatedExisting = "updatedExisting";
const std::string kFamStatusUpsertedId = "upserted";
const std::string kFamStatusAffectedCount = "n";

}  // namespace

WriteResult::WriteResult(formats::bson::Document value)
    : value_(std::move(value)) {}

size_t WriteResult::InsertedCount() const {
  return value_["insertedCount"].As<size_t>(value_["nInserted"].As<size_t>(0));
}

size_t WriteResult::MatchedCount() const {
  auto fam_status = value_[kFamStatus];
  if (fam_status.IsMissing()) {
    return value_["matchedCount"].As<size_t>(value_["nMatched"].As<size_t>(0));
  }
  if (fam_status[kFamStatusUpsertedId].IsMissing()) {
    return fam_status[kFamStatusAffectedCount].As<size_t>();
  }
  return 0;
}

size_t WriteResult::ModifiedCount() const {
  auto fam_status = value_[kFamStatus];
  if (fam_status.IsMissing()) {
    return value_["modifiedCount"].As<size_t>(
        value_["nModified"].As<size_t>(0));
  }
  if (fam_status[kFamStatusUpdatedExisting].As<bool>(false)) {
    return fam_status[kFamStatusAffectedCount].As<size_t>();
  }
  return 0;
}

size_t WriteResult::UpsertedCount() const {
  auto fam_status = value_[kFamStatus];
  if (fam_status.IsMissing()) {
    return value_["upsertedCount"].As<size_t>(
        value_["nUpserted"].As<size_t>(0));
  }
  if (!fam_status[kFamStatusUpsertedId].IsMissing()) {
    return fam_status[kFamStatusAffectedCount].As<size_t>();
  }
  return 0;
}

size_t WriteResult::DeletedCount() const {
  auto fam_status = value_[kFamStatus];
  if (fam_status.IsMissing()) {
    return value_["deletedCount"].As<size_t>(value_["nRemoved"].As<size_t>(0));
  }
  if (fam_status[kFamStatusUpdatedExisting].IsMissing()) {
    return fam_status[kFamStatusAffectedCount].As<size_t>();
  }
  return 0;
}

std::unordered_map<size_t, formats::bson::Value> WriteResult::UpsertedIds()
    const {
  std::unordered_map<size_t, formats::bson::Value> upserted_ids;

  // only one of these can be present: for FAM, update and bulk respectively
  auto fam_upserted_id = value_[kFamStatus][kFamStatusUpsertedId];
  auto upserted_id = value_["upsertedId"];
  auto upserted = value_["upserted"];
  if (!fam_upserted_id.IsMissing()) {
    upserted_ids.emplace(0, fam_upserted_id);
  } else if (!upserted_id.IsMissing()) {
    upserted_ids.emplace(0, std::move(upserted_id));
  } else if (!upserted.IsMissing()) {
    upserted_ids.reserve(upserted.GetSize());
    for (const auto& id_doc : upserted) {
      upserted_ids.emplace(id_doc["index"].As<size_t>(), id_doc["_id"]);
    }
  }
  return upserted_ids;
}

std::optional<formats::bson::Document> WriteResult::FoundDocument() const {
  auto doc = value_["value"];
  if (!doc.IsDocument()) return {};
  return doc.As<formats::bson::Document>();
}

std::unordered_map<size_t, MongoError> WriteResult::ServerErrors() const {
  std::unordered_map<size_t, MongoError> errors;

  const auto& error_values = value_["writeErrors"];
  if (error_values.IsMissing()) return errors;

  errors.reserve(error_values.GetSize());
  for (const auto& value : error_values) {
    auto it = errors.emplace(value["index"].As<size_t>(), MongoError{}).first;
    bson_set_error(it->second.GetNative(), MONGOC_ERROR_SERVER,
                   value["code"].As<uint32_t>(0), "%s",
                   value["errmsg"].As<std::string>(std::string{}).c_str());
  }
  return errors;
}

std::vector<MongoError> WriteResult::WriteConcernErrors() const {
  std::vector<MongoError> wc_errors;

  const auto& wc_error_values = value_["writeConcernErrors"];
  if (wc_error_values.IsMissing()) return wc_errors;

  wc_errors.reserve(wc_error_values.GetSize());
  for (const auto& value : wc_error_values) {
    wc_errors.emplace_back();
    bson_set_error(wc_errors.back().GetNative(), MONGOC_ERROR_WRITE_CONCERN,
                   value["code"].As<uint32_t>(0), "%s",
                   value["errmsg"].As<std::string>(std::string{}).c_str());
  }
  return wc_errors;
}

}  // namespace storages::mongo

USERVER_NAMESPACE_END
