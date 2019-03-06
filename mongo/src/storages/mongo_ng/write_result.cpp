#include <storages/mongo_ng/write_result.hpp>

#include <string>

#include <bson/bson.h>
#include <mongoc/mongoc.h>

namespace storages::mongo_ng {
namespace {

const std::string kFamStatus = "lastErrorObject";
const std::string kFamStatusUpdatedExisting = "updatedExisting";
const std::string kFamStatusUpsertedId = "upserted";
const std::string kFamStatusAffectedCount = "n";

}  // namespace

WriteResult::WriteResult(formats::bson::Document value)
    : value_(std::move(value)) {}

size_t WriteResult::InsertedCount() const {
  return value_["insertedCount"].As<size_t>(0);
}

size_t WriteResult::MatchedCount() const {
  auto fam_status = value_[kFamStatus];
  if (fam_status.IsMissing()) {
    return value_["matchedCount"].As<size_t>(0);
  }
  if (fam_status[kFamStatusUpsertedId].IsMissing()) {
    return fam_status[kFamStatusAffectedCount].As<size_t>();
  }
  return 0;
}

size_t WriteResult::ModifiedCount() const {
  auto fam_status = value_[kFamStatus];
  if (fam_status.IsMissing()) {
    return value_["modifiedCount"].As<size_t>(0);
  }
  if (fam_status[kFamStatusUpdatedExisting].As<bool>(false)) {
    return fam_status[kFamStatusAffectedCount].As<size_t>();
  }
  return 0;
}

size_t WriteResult::UpsertedCount() const {
  auto fam_status = value_[kFamStatus];
  if (fam_status.IsMissing()) {
    return value_["upsertedCount"].As<size_t>(0);
  }
  if (!fam_status[kFamStatusUpsertedId].IsMissing()) {
    return fam_status[kFamStatusAffectedCount].As<size_t>();
  }
  return 0;
}

size_t WriteResult::DeletedCount() const {
  auto fam_status = value_[kFamStatus];
  if (fam_status.IsMissing()) {
    return value_["deletedCount"].As<size_t>(0);
  }
  if (fam_status[kFamStatusUpdatedExisting].IsMissing()) {
    return fam_status[kFamStatusAffectedCount].As<size_t>();
  }
  return 0;
}

formats::bson::Value WriteResult::UpsertedId() const {
  auto fam_status = value_[kFamStatus];
  if (fam_status.IsMissing()) {
    return value_["upsertedId"];
  }
  return fam_status[kFamStatusUpsertedId];
}

boost::optional<formats::bson::Document> WriteResult::FoundDocument() const {
  auto doc = value_["value"];
  if (!doc.IsDocument()) return {};
  return doc.As<formats::bson::Document>();
}

std::vector<MongoError> WriteResult::ServerErrors() const {
  std::vector<MongoError> errors;

  const auto& error_values = value_["writeErrors"];
  if (error_values.IsMissing()) return errors;

  errors.reserve(error_values.GetSize());
  for (const auto& value : error_values) {
    errors.emplace_back();
    bson_set_error(errors.back().GetNative(), MONGOC_ERROR_SERVER,
                   // XXX: uint32_t TAXICOMMON-734
                   value["code"].As<int>(0), "%s",
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
                   // XXX: uint32_t TAXICOMMON-734
                   value["code"].As<int>(0), "%s",
                   value["errmsg"].As<std::string>(std::string{}).c_str());
  }
  return wc_errors;
}

}  // namespace storages::mongo_ng
