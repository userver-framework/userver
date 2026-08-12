#include <userver/storages/postgres/options.hpp>

#include <userver/utils/algo.hpp>
#include <userver/utils/trivial_map.hpp>
#include <userver/utils/underlying_value.hpp>

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

constexpr utils::TrivialBiMap kIsolationLevels = [](auto selector) {
    return selector()
        .Case(IsolationLevel::kReadCommitted, "read committed")
        .Case(IsolationLevel::kRepeatableRead, "repeatable read")
        .Case(IsolationLevel::kSerializable, "serializable")
        .Case(IsolationLevel::kReadUncommitted, "read uncommitted");
};

}  // namespace

USERVER_NAMESPACE::utils::StringLiteral BeginStatement(TransactionOptions opts) noexcept {
    return kStatements.TryFindByFirst(opts).value_or(kDefaultBeginStatement);
}

OptionalCommandControl GetHandlerOptionalCommandControl(
    const CommandControlByHandlerMap& map,
    std::string_view path,
    std::string_view method
) {
    const auto* const by_method_map = utils::FindOrNullptr(map, path);
    if (!by_method_map) {
        return std::nullopt;
    }
    const auto* const value = utils::FindOrNullptr(*by_method_map, method);
    if (!value) {
        return std::nullopt;
    }
    return *value;
}

OptionalCommandControl GetQueryOptionalCommandControl(
    const CommandControlByQueryMap& map,
    std::string_view query_name
) {
    const auto* value = utils::FindOrNullptr(map, query_name);
    if (!value) {
        return std::nullopt;
    }
    return *value;
}

std::string_view ToStringView(IsolationLevel lvl) { return utils::impl::EnumToStringView(lvl, kIsolationLevels); }

}  // namespace storages::postgres

USERVER_NAMESPACE_END
