#include <storages/postgres/options.hpp>

#include <unordered_map>

namespace storages {
namespace postgres {

namespace {

struct HashOptions {
  std::size_t operator()(const TransactionOptions& opts) const {
    using isolation_type = std::underlying_type<IsolationLevel>::type;
    using mode_type = std::underlying_type<TransactionOptions::Mode>::type;
    auto res = std::hash<isolation_type>()(
        static_cast<isolation_type>(opts.isolation_level));
    res <<= 1;
    res |= std::hash<mode_type>()(opts.mode);
    return res;
  }
};

const std::unordered_map<TransactionOptions, std::string, HashOptions>
    kStatements{
        {{IsolationLevel::kReadCommitted, TransactionOptions::kReadWrite},
         "begin isolation level read committed, read write"},
        {{IsolationLevel::kRepeatableRead, TransactionOptions::kReadWrite},
         "begin isolation level repeatable read, read write"},
        {{IsolationLevel::kSerializable, TransactionOptions::kReadWrite},
         "begin isolation level serializable, read write"},
        {{IsolationLevel::kReadUncommitted, TransactionOptions::kReadWrite},
         "begin isolation level read uncommitted, read write"},
        {{IsolationLevel::kReadCommitted, TransactionOptions::kReadOnly},
         "begin isolation level read committed, read only"},
        {{IsolationLevel::kRepeatableRead, TransactionOptions::kReadOnly},
         "begin isolation level repeatable read, read only"},
        {{IsolationLevel::kSerializable, TransactionOptions::kReadOnly},
         "begin isolation level serializable, read only"},
        {{IsolationLevel::kReadUncommitted, TransactionOptions::kReadOnly},
         "begin isolation level read uncommitted, read only"},
        {{IsolationLevel::kSerializable, TransactionOptions::kDeferrable},
         "begin isolation level serializable, read only, deferrable"}};

const std::string kDefaultBeginStatement = "begin";

}  // namespace

const std::string& BeginStatement(const TransactionOptions& opts) {
  auto f = kStatements.find(opts);
  if (f != kStatements.end()) {
    return f->second;
  }
  return kDefaultBeginStatement;
}

}  // namespace postgres
}  // namespace storages
