#include <storages/mongo/dist_lock_strategy.hpp>

#include <fmt/compile.h>
#include <fmt/format.h>

#include <formats/bson.hpp>
#include <hostinfo/blocking/get_hostname.hpp>
#include <logging/log.hpp>
#include <storages/mongo/exception.hpp>

#include <formats/bson/serialize.hpp>

namespace storages::mongo {

namespace {
namespace fields {

const std::string kId = "_id";
const std::string kLockedTill = "t";
const std::string kOwner = "o";

}  // namespace fields

std::string MakeOwnerId(const std::string& prefix, const std::string& locker) {
  return fmt::format(FMT_COMPILE("{}:{}"), prefix, locker);
}

}  // namespace

DistLockStrategy::DistLockStrategy(Collection collection, std::string lock_name)
    : DistLockStrategy(std::move(collection), std::move(lock_name),
                       hostinfo::blocking::GetRealHostName()) {}

DistLockStrategy::DistLockStrategy(Collection collection, std::string lock_name,
                                   std::string owner)
    : collection_(std::move(collection)),
      lock_name_(std::move(lock_name)),
      owner_prefix_(std::move(owner)) {}

void DistLockStrategy::Acquire(std::chrono::milliseconds lock_ttl,
                               const std::string& locker_id) {
  namespace bson = formats::bson;

  const auto now = utils::datetime::Now();
  const auto expiration_time = now + lock_ttl;

  const auto owner = MakeOwnerId(owner_prefix_, locker_id);

  auto query = bson::MakeDoc(
      fields::kId, lock_name_, "$or",
      bson::MakeArray(
          bson::MakeDoc(fields::kLockedTill, bson::MakeDoc("$lte", now)),
          bson::MakeDoc(fields::kOwner, owner)));
  auto update =
      bson::MakeDoc("$set", bson::MakeDoc(fields::kLockedTill, expiration_time,
                                          fields::kOwner, owner));

  try {
    auto lock = collection_
                    .FindAndModify(std::move(query), update, options::Upsert{},
                                   options::ReturnNew{})
                    .FoundDocument();
    if (!lock) {
      throw dist_lock::LockIsAcquiredByAnotherHostException();
    }
  } catch (const DuplicateKeyException& exc) {
    throw dist_lock::LockIsAcquiredByAnotherHostException();
  } catch (const MongoException& exc) {
    LOG_WARNING() << "owner " << owner << " could not acquire a lock "
                  << lock_name_ << " because of mongo error: " << exc;
    throw;
  }
}

void DistLockStrategy::Release(const std::string& locker_id) {
  namespace bson = formats::bson;

  const auto owner = MakeOwnerId(owner_prefix_, locker_id);

  auto query = bson::MakeDoc(fields::kId, lock_name_, fields::kOwner, owner);
  size_t deleted_count = 0;
  try {
    deleted_count = collection_.FindAndRemove(std::move(query)).DeletedCount();
  } catch (const std::exception& e) {
    LOG_WARNING() << "owner " << owner << " could not release a lock "
                  << lock_name_ << " because of mongo error: " << e.what();
    return;
  }
  if (!deleted_count) {
    LOG_WARNING() << "owner " << owner << " could not release a lock "
                  << lock_name_;
  }
}

}  // namespace storages::mongo
