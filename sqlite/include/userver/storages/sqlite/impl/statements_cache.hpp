#pragma once

#include <memory>

#include <userver/cache/lru_map.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/utils/str_icase.hpp>

#include <userver/storages/sqlite/impl/native_handler.hpp>
#include <userver/storages/sqlite/impl/statement.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

class StatementsCache final {
public:
    StatementsCache(const NativeHandler& db_handler, std::size_t capacity);
    ~StatementsCache();

    StatementsCache(StatementsCache&&) noexcept = default;
    StatementsCache(const StatementsCache&) = delete;

    StatementsCache& operator=(StatementsCache&&) noexcept = delete;
    StatementsCache& operator=(const StatementsCache&) = delete;

    std::shared_ptr<Statement> PrepareStatement(const std::string& statement);

private:
    const NativeHandler& db_handler_;

    cache::LruMap<std::string, std::shared_ptr<Statement>, utils::StrIcaseHash, utils::StrIcaseEqual> cache_;
};

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
