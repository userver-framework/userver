#include <userver/storages/postgres/options.hpp>

#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

namespace {

constexpr utils::TrivialBiMap kStatements = [](auto selector) {
    return selector()
        .Case(
            TransactionOptions{IsolationLevel::kReadCommitted, TransactionOptions::kReadWrite},
            "begin isolation level read committed, read write"
        )
        .Case(
            TransactionOptions{IsolationLevel::kRepeatableRead, TransactionOptions::kReadWrite},
            "begin isolation level repeatable read, read write"
        )
        .Case(
            TransactionOptions{IsolationLevel::kSerializable, TransactionOptions::kReadWrite},
            "begin isolation level serializable, read write"
        )
        .Case(
            TransactionOptions{IsolationLevel::kReadUncommitted, TransactionOptions::kReadWrite},
            "begin isolation level read uncommitted, read write"
        )
        .Case(
            TransactionOptions{IsolationLevel::kReadCommitted, TransactionOptions::kReadOnly},
            "begin isolation level read committed, read only"
        )
        .Case(
            TransactionOptions{IsolationLevel::kRepeatableRead, TransactionOptions::kReadOnly},
            "begin isolation level repeatable read, read only"
        )
        .Case(
            TransactionOptions{IsolationLevel::kSerializable, TransactionOptions::kReadOnly},
            "begin isolation level serializable, read only"
        )
        .Case(
            TransactionOptions{IsolationLevel::kReadUncommitted, TransactionOptions::kReadOnly},
            "begin isolation level read uncommitted, read only"
        )
        .Case(
            TransactionOptions{IsolationLevel::kSerializable, TransactionOptions::kDeferrable},
            "begin isolation level serializable, read only, deferrable"
        );
};

constexpr utils::StringLiteral kDefaultBeginStatement = "begin";

}  // namespace

USERVER_NAMESPACE::utils::StringLiteral BeginStatement(TransactionOptions opts) noexcept {
    return kStatements.TryFindByFirst(opts).value_or(kDefaultBeginStatement);
}

OptionalCommandControl GetHandlerOptionalCommandControl(
    const CommandControlByHandlerMap& map,
    std::string_view path,
    std::string_view method
) {
    const auto* const by_method_map = utils::impl::FindTransparentOrNullptr(map, path);
    if (!by_method_map) return std::nullopt;
    const auto* const value = utils::impl::FindTransparentOrNullptr(*by_method_map, method);
    if (!value) return std::nullopt;
    return *value;
}

OptionalCommandControl
GetQueryOptionalCommandControl(const CommandControlByQueryMap& map, const std::string& query_name) {
    auto it = map.find(query_name);
    if (it == map.end()) return std::nullopt;
    return it->second;
}

}  // namespace storages::postgres

USERVER_NAMESPACE_END
