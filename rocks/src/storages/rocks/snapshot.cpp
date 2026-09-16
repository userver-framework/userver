#include <userver/storages/rocks/snapshot.hpp>

#include <rocksdb/options.h>

#include <userver/engine/async.hpp>
#include <userver/storages/rocks/cursor.hpp>
#include <userver/storages/rocks/exception.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

Snapshot::Snapshot(std::shared_ptr<rocksdb::DB> db, engine::TaskProcessor& tp, const rocksdb::Snapshot* snap)
    : db_(std::move(db)),
      tp_(&tp),
      snap_(snap)
{}

Snapshot::Snapshot(Snapshot&& other) noexcept
    : db_(std::move(other.db_)), tp_(other.tp_), snap_(other.snap_)
{
    other.snap_ = nullptr;
}

Snapshot& Snapshot::operator=(Snapshot&& other) noexcept {
    if (this != &other) {
        if (snap_ != nullptr) {
            db_->ReleaseSnapshot(snap_);
        }
        db_ = std::move(other.db_);
        tp_ = other.tp_;
        snap_ = other.snap_;
        other.snap_ = nullptr;
    }
    return *this;
}

Snapshot::~Snapshot() {
    if (snap_ != nullptr) {
        db_->ReleaseSnapshot(snap_);
    }
}

std::optional<std::string> Snapshot::Get(std::string_view key) const {
    return engine::AsyncNoTracing(
               *tp_,
               [this, key]() -> std::optional<std::string> {
                   rocksdb::ReadOptions ro;
                   ro.snapshot = snap_;

                   std::string value;
                   const rocksdb::Status status = db_->Get(ro, key, &value);

                   if (status.IsNotFound()) {
                       return std::nullopt;
                   }
                   if (!status.ok()) {
                       throw RequestFailedException("Snapshot::Get", status.ToString());
                   }
                   return value;
               }
    ).Get();
}

std::vector<std::optional<std::string>> Snapshot::GetMany(const std::vector<std::string_view>& keys) const {
    return engine::AsyncNoTracing(
               *tp_,
               [this, &keys] {
                   rocksdb::ReadOptions ro;
                   ro.snapshot = snap_;

                   std::vector<rocksdb::Slice> slices;
                   slices.reserve(keys.size());
                   for (const auto k : keys) {
                       slices.emplace_back(k.data(), k.size());
                   }

                   std::vector<std::string> raw_values;
                   const std::vector<rocksdb::Status> statuses = db_->MultiGet(ro, slices, &raw_values);

                   std::vector<std::optional<std::string>> result;
                   result.reserve(statuses.size());
                   for (std::size_t i = 0; i < statuses.size(); ++i) {
                       if (statuses[i].IsNotFound()) {
                           result.push_back(std::nullopt);
                       } else if (!statuses[i].ok()) {
                           throw RequestFailedException("Snapshot::GetMany", statuses[i].ToString());
                       } else {
                           result.push_back(std::move(raw_values[i]));
                       }
                   }
                   return result;
               }
    ).Get();
}

std::vector<std::optional<std::string>> Snapshot::GetMany(const std::vector<std::string>& keys) const {
    std::vector<std::string_view> views;
    views.reserve(keys.size());
    for (const auto& k : keys) {
        views.emplace_back(k);
    }
    return GetMany(views);
}

Cursor Snapshot::Scan(std::string_view prefix) && {
    const rocksdb::Snapshot* snap = snap_;
    snap_ = nullptr;
    return Cursor(std::move(db_), *tp_, snap, std::string(prefix));
}

}  // namespace storages::rocks

USERVER_NAMESPACE_END
