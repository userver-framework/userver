#pragma once

/// @file userver/storages/rocks/db_builder.hpp
/// @brief @copybrief storages::rocks::DbBuilder

#include <memory>
#include <string>
#include <unordered_set>

#include <userver/formats/parse/to.hpp>
#include <userver/storages/rocks/client.hpp>
#include <userver/storages/rocks/transaction.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

/// @brief Typed write-side counterpart to Map<Key, Value>.
///
/// Opens a transaction, accepts Emplace() calls, and on Commit() atomically
/// replaces the entire DB content: all keys not passed to Emplace() are deleted,
/// and all emplaced pairs are written.
///
/// If Commit() is never called, the transaction is rolled back automatically
/// on destruction — the DB is left unchanged.
///
/// Keys are serialised via @c ToString(key), values via @c ToString(value).
///
/// @warning The client must have been opened with TransactionType::kPessimistic
/// or TransactionType::kOptimistic.
template <typename Key, typename Value>
class DbBuilder final {
public:
    /// Begins a transaction on @p client.
    ///
    /// @throws storages::rocks::Exception if the client has no transaction support.
    explicit DbBuilder(Client& client)
        : client_(&client),
          txn_(std::make_unique<Transaction>(client.BeginTransaction()))
    {}

    DbBuilder(const DbBuilder&) = delete;
    DbBuilder& operator=(const DbBuilder&) = delete;
    DbBuilder(DbBuilder&&) noexcept = default;
    DbBuilder& operator=(DbBuilder&&) noexcept = default;

    ~DbBuilder() = default;

    /// Serialises @p key / @p value and buffers a Put in the transaction.
    /// If the same key is emplaced more than once, the last value wins.
    void Emplace(const Key& key, const Value& value) {
        auto key_str = ToString(key);
        emplaced_keys_.insert(key_str);
        txn_->Put(key_str, ToString(value));
    }

    /// Scans all existing keys, deletes those not in the emplace set,
    /// then commits the transaction atomically.
    ///
    /// @throws storages::rocks::WriteConflictException on optimistic conflict.
    /// @throws storages::rocks::RequestFailedException on other commit errors.
    void Commit() {
        DeleteNonEmplacedKeys();
        txn_->Commit();
    }

private:
    void DeleteNonEmplacedKeys() {
        auto cursor = client_->Scan();
        while (true) {
            auto batch = cursor.FetchBatch(100);
            if (batch.empty()) {
                break;
            }
            for (const auto& kv : batch) {
                if (!emplaced_keys_.count(kv.key)) {
                    txn_->Delete(kv.key);
                }
            }
        }
    }

    Client* client_;
    std::unique_ptr<Transaction> txn_;
    std::unordered_set<std::string> emplaced_keys_;
};

}  // namespace storages::rocks

USERVER_NAMESPACE_END
