#include <gtest/gtest.h>
#include <array>
#include <atomic>
#include <chrono>
#include <concepts>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <storages/odbc/detail/connection.hpp>
#include <storages/odbc/detail/tracing.hpp>
#include <string>
#include <string_view>
#include <type_traits>
#include <userver/storages/odbc.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>
#include <vector>

#include <userver/storages/odbc/tests/utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::tests::mapping {

/// [ODBC mapped domain types]
struct StrongId final {
    std::int64_t value;

    bool operator==(const StrongId&) const = default;
};

enum class TextState { kReady, kPaused };
enum class NativeState : std::uint16_t { kOne = 1, kTwo = 2 };

struct Record final {
    StrongId id;
    std::string text;
    std::optional<StrongId> parent;
    TextState state;

    bool operator==(const Record&) const = default;
};
/// [ODBC mapped domain types]

struct MappedPair final {
    std::int32_t first;
    std::int32_t second;
};

struct ToOnlyAggregate final {
    std::int64_t value;
};

struct FromOnlyAggregate final {
    std::int64_t value;
};

struct InvalidBoundAggregate final {
    std::int64_t value;
};

struct CvBoundAggregate final {
    std::int64_t value;
};

struct ReferenceBoundAggregate final {
    std::int64_t value;
};

struct EnumBoundAggregate final {
    std::int64_t value;
};

struct MissingBoundAggregate final {
    std::int64_t value;
};

struct BadToAggregate final {
    std::int64_t value;
};

struct BadFromAggregate final {
    std::int64_t value;
};

struct MappedChainAggregate final {
    std::int64_t value;
};

struct OptionalMappingMarker final {
    std::int64_t value;
};

struct CountingMapped final {
    std::int64_t value;

    bool operator==(const CountingMapped&) const = default;
};

struct NonDefaultMapped final {
    NonDefaultMapped() = delete;
    explicit NonDefaultMapped(std::int64_t value)
        : value{value}
    {}

    std::int64_t value;
};

struct ThrowingInput final {};
struct ThrowingOutput final {};

struct MismatchOutput final {
    std::int64_t value;
};

struct InvalidMappedAggregate final {
    std::int64_t first;
    std::int64_t second;
};

struct NativeAggregate final {
    std::int32_t number;
    std::string text;
};

struct ContainsNativeAggregate final {
    NativeAggregate nested;
    std::int32_t tail;
};

struct ContainsMappedAggregate final {
    MappedPair nested;
    std::int32_t tail;
};

struct EmptyAggregate final {};

struct ReferenceAggregate final {
    std::int32_t& value;
};

class PrivateAggregate final {
public:
    int GetValue() const { return value_; }
    void SetValue(int value) { value_ = value; }

private:
    int value_{0};
};

union UnionAggregate {
    std::int32_t value;
};

struct AggregateBase {
    std::int32_t base;
};

struct DerivedAggregate final : AggregateBase {
    std::int32_t value;
};

struct EmptyBase {};

struct DerivedEmptyBaseAggregate final : EmptyBase {
    std::int32_t value;
};

struct CountingAggregate final {
    CountingMapped first;
    std::int32_t middle;
    CountingMapped last;
};

struct StrongGuaranteeAggregate final {
    std::int32_t first;
    ThrowingInput second;
};

struct NonDefaultLaterAggregate final {
    std::int32_t first;
    NonDefaultMapped second;
};

struct LargeEmptyBaseRejectedAggregate final : EmptyBase {
    std::array<std::int32_t, 300> payload;
};

inline std::atomic<int> g_to_calls{0};
inline std::atomic<int> g_from_calls{0};
inline std::atomic<int> g_mismatch_from_calls{0};

}  // namespace storages::odbc::tests::mapping

namespace storages::odbc::io {

/// [ODBC custom type mapping]
template <>
struct CppToOdbc<tests::mapping::StrongId> final {
    using BoundType = std::int64_t;

    static BoundType ToOdbc(const tests::mapping::StrongId& value) { return value.value; }
    static tests::mapping::StrongId FromOdbc(BoundType value) { return {value}; }
};

template <>
struct CppToOdbc<tests::mapping::TextState> final {
    using BoundType = std::string;

    static BoundType ToOdbc(const tests::mapping::TextState& value) {
        return value == tests::mapping::TextState::kReady ? "ready" : "paused";
    }

    static tests::mapping::TextState FromOdbc(BoundType value) {
        if (value == "ready") {
            return tests::mapping::TextState::kReady;
        }
        if (value == "paused") {
            return tests::mapping::TextState::kPaused;
        }
        throw std::runtime_error("unknown mapped text state");
    }
};
/// [ODBC custom type mapping]

template <>
struct CppToOdbc<tests::mapping::MappedPair> final {
    using BoundType = std::string;

    static BoundType ToOdbc(const tests::mapping::MappedPair& value) {
        return std::to_string(value.first) + ":" + std::to_string(value.second);
    }
    static tests::mapping::MappedPair FromOdbc(BoundType) { return {1, 2}; }
};

template <>
struct CppToOdbc<tests::mapping::ToOnlyAggregate> final {
    using BoundType = std::int64_t;
    static BoundType ToOdbc(const tests::mapping::ToOnlyAggregate& value) { return value.value; }
};

template <>
struct CppToOdbc<tests::mapping::FromOnlyAggregate> final {
    using BoundType = std::int64_t;
    static tests::mapping::FromOnlyAggregate FromOdbc(BoundType value) { return {value}; }
};

template <>
struct CppToOdbc<tests::mapping::InvalidBoundAggregate> final {
    using BoundType = std::optional<std::int64_t>;
};

template <>
struct CppToOdbc<tests::mapping::CvBoundAggregate> final {
    using BoundType = const std::int64_t;
};

template <>
struct CppToOdbc<tests::mapping::ReferenceBoundAggregate> final {
    using BoundType = std::int64_t&;
};

template <>
struct CppToOdbc<tests::mapping::EnumBoundAggregate> final {
    using BoundType = tests::mapping::NativeState;
};

template <>
struct CppToOdbc<tests::mapping::MissingBoundAggregate> final {};

template <>
struct CppToOdbc<tests::mapping::BadToAggregate> final {
    using BoundType = std::int64_t;
    static BoundType ToOdbc(tests::mapping::BadToAggregate) { return 0; }
};

template <>
struct CppToOdbc<tests::mapping::BadFromAggregate> final {
    using BoundType = std::int64_t;
    static tests::mapping::BadFromAggregate FromOdbc(const BoundType&) { return {0}; }
};

template <>
struct CppToOdbc<tests::mapping::MappedChainAggregate> final {
    using BoundType = tests::mapping::StrongId;
};

template <>
struct CppToOdbc<std::optional<tests::mapping::OptionalMappingMarker>> final {
    using BoundType = std::int64_t;
    static BoundType ToOdbc(const std::optional<tests::mapping::OptionalMappingMarker>&) { return 0; }
    static std::optional<tests::mapping::OptionalMappingMarker> FromOdbc(BoundType value) {
        return tests::mapping::OptionalMappingMarker{value};
    }
};

template <>
struct CppToOdbc<tests::mapping::CountingMapped> final {
    using BoundType = std::int64_t;
    static BoundType ToOdbc(const tests::mapping::CountingMapped& value) {
        ++tests::mapping::g_to_calls;
        return value.value;
    }
    static tests::mapping::CountingMapped FromOdbc(BoundType value) {
        ++tests::mapping::g_from_calls;
        return {value};
    }
};

template <>
struct CppToOdbc<tests::mapping::NonDefaultMapped> final {
    using BoundType = std::int64_t;
    static BoundType ToOdbc(const tests::mapping::NonDefaultMapped& value) { return value.value; }
    static tests::mapping::NonDefaultMapped FromOdbc(BoundType value) {
        return tests::mapping::NonDefaultMapped{value};
    }
};

template <>
struct CppToOdbc<tests::mapping::ThrowingInput> final {
    using BoundType = std::int64_t;
    static BoundType ToOdbc(const tests::mapping::ThrowingInput&) { throw std::runtime_error("input conversion"); }
};

template <>
struct CppToOdbc<tests::mapping::ThrowingOutput> final {
    using BoundType = std::int64_t;
    static tests::mapping::ThrowingOutput FromOdbc(BoundType) { throw std::runtime_error("output conversion"); }
};

template <>
struct CppToOdbc<tests::mapping::MismatchOutput> final {
    using BoundType = std::int64_t;
    static tests::mapping::MismatchOutput FromOdbc(BoundType value) {
        ++tests::mapping::g_mismatch_from_calls;
        return {value};
    }
};

template <>
struct CppToOdbc<tests::mapping::InvalidMappedAggregate> final {
    using BoundType = std::string_view;
};

}  // namespace storages::odbc::io

namespace storages::odbc::tests {

using ClusterParameterStoreExecute = ResultSet (Cluster::*)(ClusterHostTypeFlags, const Query&, const ParameterStore&);
using ClusterParameterStoreExecuteWithCommandControl =
    ResultSet (Cluster::*)(ClusterHostTypeFlags, OptionalCommandControl, const Query&, const ParameterStore&);
using TransactionParameterStoreExecute = ResultSet (Transaction::*)(const Query&, const ParameterStore&);
using TransactionParameterStoreExecuteWithCommandControl =
    ResultSet (Transaction::*)(OptionalCommandControl, const Query&, const ParameterStore&);
using ClusterParameterStoreExecuteCursor =
    Cursor (Cluster::*)(ClusterHostTypeFlags, const Query&, const ParameterStore&);
using ClusterParameterStoreExecuteCursorWithCommandControl =
    Cursor (Cluster::*)(ClusterHostTypeFlags, OptionalCommandControl, const Query&, const ParameterStore&);
using TransactionParameterStoreExecuteCursor = Cursor (Transaction::*)(const Query&, const ParameterStore&);
using TransactionParameterStoreExecuteCursorWithCommandControl =
    Cursor (Transaction::*)(OptionalCommandControl, const Query&, const ParameterStore&);

template <typename T>
concept ParameterStorePushable = requires(ParameterStore& store, const T& value) { store.PushBack(value); };

template <typename T>
concept ParameterListBuildable = requires(const T& value) { impl::MakeParameterList(value); };

template <typename T>
concept BulkRowPushable = requires(BulkParameterStore& store, const T& value) { store.PushBackRow(value); };

template <typename T>
concept ClusterParameterExecutable = requires(Cluster& cluster, const Query& query, const T& value) {
    cluster.Execute(ClusterHostType::kMaster, query, value);
};

struct UnsupportedParameter final {};

static_assert(!std::is_copy_constructible_v<ParameterStore>);
static_assert(!std::is_copy_assignable_v<ParameterStore>);
static_assert(std::is_nothrow_move_constructible_v<ParameterStore>);
static_assert(std::is_nothrow_move_assignable_v<ParameterStore>);
static_assert(!std::is_copy_constructible_v<Cursor>);
static_assert(!std::is_copy_assignable_v<Cursor>);
static_assert(std::is_nothrow_move_constructible_v<Cursor>);
static_assert(std::is_nothrow_move_assignable_v<Cursor>);
static_assert(std::is_nothrow_destructible_v<Cursor>);
static_assert(requires(ParameterStore& store, const std::optional<std::int32_t>& value) {
    {
        store.PushBack(value)
    } -> std::same_as<ParameterStore&>;
    store.PushBack("string literal");
});
static_assert(ParameterStorePushable<std::nullptr_t>);
static_assert(ParameterStorePushable<std::nullopt_t>);
static_assert(ParameterStorePushable<const char*>);
static_assert(ParameterStorePushable<std::string_view>);
static_assert(ParameterStorePushable<std::optional<std::string>>);
static_assert(ParameterStorePushable<Bytes>);
static_assert(ParameterStorePushable<Date>);
static_assert(ParameterStorePushable<Time>);
static_assert(ParameterStorePushable<Timestamp>);
static_assert(ParameterStorePushable<Decimal<9, 4>>);
static_assert(ParameterStorePushable<std::optional<Bytes>>);
static_assert(ParameterStorePushable<std::optional<Decimal<9, 4>>>);
static_assert(!ParameterStorePushable<void*>);
static_assert(!ParameterStorePushable<UnsupportedParameter>);
static_assert(!ParameterStorePushable<std::optional<std::vector<int>>>);
static_assert(io::traits::kHasMappingDeclaration<mapping::StrongId>);
static_assert(io::traits::kHasValidBoundType<const mapping::StrongId&>);
static_assert(io::traits::kHasToOdbc<mapping::StrongId>);
static_assert(io::traits::kHasFromOdbc<mapping::StrongId>);
static_assert(io::traits::kHasToOdbc<mapping::ToOnlyAggregate>);
static_assert(!io::traits::kHasFromOdbc<mapping::ToOnlyAggregate>);
static_assert(!io::traits::kHasToOdbc<mapping::FromOnlyAggregate>);
static_assert(io::traits::kHasFromOdbc<mapping::FromOnlyAggregate>);
static_assert(!io::traits::kHasValidBoundType<mapping::InvalidBoundAggregate>);
static_assert(!io::traits::kHasValidBoundType<mapping::CvBoundAggregate>);
static_assert(!io::traits::kHasValidBoundType<mapping::ReferenceBoundAggregate>);
static_assert(!io::traits::kHasValidBoundType<mapping::EnumBoundAggregate>);
static_assert(!io::traits::kHasValidBoundType<mapping::MissingBoundAggregate>);
static_assert(!io::traits::kHasValidBoundType<mapping::MappedChainAggregate>);
static_assert(!io::traits::kHasToOdbc<mapping::BadToAggregate>);
static_assert(!io::traits::kHasFromOdbc<mapping::BadFromAggregate>);
static_assert(!io::traits::kHasValidBoundType<std::optional<mapping::OptionalMappingMarker>>);
static_assert(impl::kIsParameterStoreValue<mapping::StrongId>);
static_assert(impl::kIsFieldAsType<mapping::StrongId>);
static_assert(impl::kIsParameterStoreValue<mapping::TextState>);
static_assert(impl::kIsFieldAsType<mapping::TextState>);
static_assert(impl::kIsParameterStoreValue<mapping::NativeState>);
static_assert(!impl::kIsFieldAsType<mapping::NativeState>);
static_assert(impl::kIsParameterAggregate<mapping::Record>);
static_assert(impl::kIsResultAggregate<mapping::Record>);
static_assert(impl::ParameterArgumentWidth<mapping::Record>() == 4);
static_assert(impl::kIsParameterStoreValue<mapping::MappedPair>);
static_assert(!impl::kIsParameterAggregate<mapping::MappedPair>);
static_assert(impl::ParameterArgumentWidth<mapping::MappedPair>() == 1);
static_assert(impl::kIsParameterStoreValue<mapping::ToOnlyAggregate>);
static_assert(!impl::kIsParameterAggregate<mapping::ToOnlyAggregate>);
static_assert(!impl::kIsResultAggregate<mapping::ToOnlyAggregate>);
static_assert(!impl::kIsParameterAggregate<mapping::FromOnlyAggregate>);
static_assert(impl::kIsFieldAsType<mapping::FromOnlyAggregate>);
static_assert(!impl::kIsParameterArgument<mapping::InvalidMappedAggregate>);
static_assert(!impl::kIsResultAggregate<mapping::InvalidMappedAggregate>);
static_assert(impl::kIsParameterAggregate<mapping::NativeAggregate>);
static_assert(impl::kIsResultAggregate<mapping::NativeAggregate>);
static_assert(impl::kIsParameterAggregate<mapping::NonDefaultLaterAggregate>);
static_assert(impl::kIsResultAggregate<mapping::NonDefaultLaterAggregate>);
static_assert(!impl::kIsParameterArgument<mapping::LargeEmptyBaseRejectedAggregate>);
static_assert(!impl::kIsResultValue<mapping::LargeEmptyBaseRejectedAggregate>);
static_assert(!impl::kIsParameterArgument<mapping::ContainsNativeAggregate>);
static_assert(!impl::kIsResultValue<mapping::ContainsNativeAggregate>);
static_assert(impl::kIsParameterAggregate<mapping::ContainsMappedAggregate>);
static_assert(impl::kIsResultAggregate<mapping::ContainsMappedAggregate>);
static_assert(impl::ParameterArgumentWidth<mapping::ContainsMappedAggregate>() == 2);
static_assert(!impl::kIsParameterArgument<mapping::EmptyAggregate>);
static_assert(!impl::kIsResultValue<mapping::EmptyAggregate>);
static_assert(!impl::kIsParameterArgument<mapping::ReferenceAggregate>);
static_assert(!impl::kIsResultValue<mapping::ReferenceAggregate>);
static_assert(!impl::kIsParameterArgument<mapping::PrivateAggregate>);
static_assert(!impl::kIsResultValue<mapping::PrivateAggregate>);
static_assert(!impl::kIsParameterArgument<mapping::UnionAggregate>);
static_assert(!impl::kIsResultValue<mapping::UnionAggregate>);
static_assert(!impl::kIsParameterArgument<mapping::DerivedAggregate>);
static_assert(!impl::kIsResultValue<mapping::DerivedAggregate>);
static_assert(!impl::kIsParameterArgument<mapping::DerivedEmptyBaseAggregate>);
static_assert(!impl::kIsResultValue<mapping::DerivedEmptyBaseAggregate>);
static_assert(!impl::kIsParameterArgument<std::optional<mapping::NativeAggregate>>);
static_assert(!impl::kIsResultValue<std::optional<mapping::NativeAggregate>>);
static_assert(!impl::kIsParameterArgument<std::optional<mapping::OptionalMappingMarker>>);
static_assert(!impl::kIsResultValue<std::optional<mapping::OptionalMappingMarker>>);
static_assert(ParameterListBuildable<mapping::Record>);
static_assert(ParameterStorePushable<mapping::Record>);
static_assert(BulkRowPushable<mapping::Record>);
static_assert(ClusterParameterExecutable<mapping::Record>);
static_assert(!ParameterListBuildable<mapping::ContainsNativeAggregate>);
static_assert(!ParameterStorePushable<mapping::ContainsNativeAggregate>);
static_assert(!BulkRowPushable<mapping::ContainsNativeAggregate>);
static_assert(!ClusterParameterExecutable<mapping::ContainsNativeAggregate>);
static_assert(requires {
    static_cast<ClusterParameterStoreExecute>(&Cluster::Execute);
    static_cast<ClusterParameterStoreExecuteWithCommandControl>(&Cluster::Execute);
    static_cast<TransactionParameterStoreExecute>(&Transaction::Execute);
    static_cast<TransactionParameterStoreExecuteWithCommandControl>(&Transaction::Execute);
    static_cast<ClusterParameterStoreExecuteCursor>(&Cluster::ExecuteCursor);
    static_cast<ClusterParameterStoreExecuteCursorWithCommandControl>(&Cluster::ExecuteCursor);
    static_cast<TransactionParameterStoreExecuteCursor>(&Transaction::ExecuteCursor);
    static_cast<TransactionParameterStoreExecuteCursorWithCommandControl>(&Transaction::ExecuteCursor);
});

UTEST(OdbcTypeMapping, FlattensInOrderCallsHooksOnceAndKeepsStoresStrong) {
    mapping::g_to_calls = 0;
    const auto parameters =
        impl::MakeParameterList(std::int32_t{10}, mapping::CountingAggregate{{20}, 30, {40}}, std::string{"tail"});
    ASSERT_EQ(parameters.size(), 5);
    EXPECT_EQ(parameters[0].Get<std::int64_t>(), 10);
    EXPECT_EQ(parameters[1].Get<std::int64_t>(), 20);
    EXPECT_EQ(parameters[2].Get<std::int64_t>(), 30);
    EXPECT_EQ(parameters[3].Get<std::int64_t>(), 40);
    EXPECT_EQ(parameters[4].Get<std::string>(), "tail");
    EXPECT_EQ(mapping::g_to_calls, 2);

    const auto enum_parameters = impl::MakeParameterList(mapping::NativeState::kTwo, mapping::TextState::kPaused);
    ASSERT_EQ(enum_parameters.size(), 2);
    EXPECT_EQ(enum_parameters[0].Get<std::uint64_t>(), 2);
    EXPECT_EQ(enum_parameters[1].Get<std::string>(), "paused");

    ParameterStore store;
    store.PushBack(std::int32_t{7});
    EXPECT_EQ(store.Size(), 1);
    UEXPECT_THROW(store.PushBack(mapping::StrongGuaranteeAggregate{8, {}}), std::runtime_error);
    EXPECT_EQ(store.Size(), 1);

    BulkParameterStore bulk;
    bulk.PushBackRow(mapping::NativeAggregate{1, "one"}, mapping::StrongId{2});
    EXPECT_EQ(bulk.RowsCount(), 1);
    EXPECT_EQ(bulk.ColumnsCount(), 3);
    bulk.PushBackRow(mapping::NativeAggregate{3, "three"}, mapping::StrongId{4});
    EXPECT_EQ(bulk.RowsCount(), 2);
    EXPECT_EQ(bulk.ColumnsCount(), 3);
}

UTEST(OdbcTypeMapping, AggregateAndMappedScalarsHaveExecutionParity) {
    auto cluster = MakeCluster();
    const std::string hostile = "Robert'); DROP TABLE odbc_mapping_rows;--";
    const mapping::Record input{
        mapping::StrongId{41},
        hostile,
        std::nullopt,
        mapping::TextState::kReady,
    };
    const Query echo_query{"SELECT ?::bigint, ?::text, ?::bigint, ?::text"};

    /// [ODBC aggregate parameters and mapped results]
    const auto direct = cluster.Execute(ClusterHostType::kMaster, echo_query, input).AsSingleRow<mapping::Record>();
    EXPECT_EQ(direct, input);

    ParameterStore parameters;
    parameters.PushBack(input);
    EXPECT_EQ(parameters.Size(), 4);
    const auto
        stored = cluster.Execute(ClusterHostType::kMaster, echo_query, parameters).AsSingleRow<mapping::Record>();
    EXPECT_EQ(stored, input);
    /// [ODBC aggregate parameters and mapped results]

    auto cursor = cluster.ExecuteCursor(ClusterHostType::kMaster, echo_query, input);
    const auto cursor_chunk = cursor.Fetch(2).AsSingleRow<mapping::Record>();
    EXPECT_EQ(cursor_chunk, input);
    EXPECT_TRUE(cursor.Done());

    EXPECT_EQ(
        cluster.Execute(ClusterHostType::kMaster, "SELECT ?::bigint", mapping::StrongId{77})
            .AsSingleRow<mapping::StrongId>(),
        (mapping::StrongId{77})
    );
    EXPECT_EQ(
        cluster.Execute(ClusterHostType::kMaster, "SELECT ?::text", mapping::TextState::kPaused)
            .AsSingleRow<mapping::TextState>(),
        mapping::TextState::kPaused
    );
    EXPECT_FALSE(cluster.Execute(ClusterHostType::kMaster, "SELECT NULL::bigint")
                     .AsSingleRow<std::optional<mapping::StrongId>>()
                     .has_value());

    mapping::g_to_calls = 0;
    mapping::g_from_calls = 0;
    const auto mapped_null =
        cluster.Execute(ClusterHostType::kMaster, "SELECT ?::bigint", std::optional<mapping::CountingMapped>{})
            .AsSingleRow<std::optional<mapping::CountingMapped>>();
    EXPECT_FALSE(mapped_null);
    EXPECT_EQ(mapping::g_to_calls, 0);
    EXPECT_EQ(mapping::g_from_calls, 0);
    const auto mapped_value =
        cluster
            .Execute(
                ClusterHostType::kMaster,
                "SELECT ?::bigint",
                std::optional<mapping::CountingMapped>{mapping::CountingMapped{88}}
            )
            .AsSingleRow<std::optional<mapping::CountingMapped>>();
    EXPECT_EQ(mapped_value, std::optional<mapping::CountingMapped>{mapping::CountingMapped{88}});
    EXPECT_EQ(mapping::g_to_calls, 1);
    EXPECT_EQ(mapping::g_from_calls, 1);

    auto transaction = cluster.Begin(ClusterHostType::kMaster);
    transaction
        .Execute("CREATE TEMP TABLE odbc_mapping_rows(id BIGINT, text_value TEXT, parent BIGINT NULL, state TEXT)");
    BulkParameterStore rows;
    rows.PushBackRow(input).PushBackRow(mapping::Record{
        mapping::StrongId{42},
        "second",
        mapping::StrongId{41},
        mapping::TextState::kPaused,
    });
    EXPECT_EQ(rows.ColumnsCount(), 4);
    const auto bulk = transaction.ExecuteBulk("INSERT INTO odbc_mapping_rows VALUES (?, ?, ?, ?)", rows);
    EXPECT_EQ(bulk.Requested(), 2);
    const auto roundtrip =
        transaction.Execute("SELECT id, text_value, parent, state FROM odbc_mapping_rows ORDER BY id")
            .AsContainer<std::vector<mapping::Record>>();
    ASSERT_EQ(roundtrip.size(), 2);
    EXPECT_EQ(roundtrip[0], input);
    EXPECT_EQ(roundtrip[1].parent, std::optional<mapping::StrongId>{mapping::StrongId{41}});
    transaction.Rollback();

    UEXPECT_THROW(
        cluster.Execute(ClusterHostType::kMaster, "SELECT ?::bigint", mapping::ThrowingInput{}),
        std::runtime_error
    );
    UEXPECT_THROW(
        cluster.Execute(ClusterHostType::kMaster, "SELECT 1::bigint").AsSingleRow<mapping::ThrowingOutput>(),
        std::runtime_error
    );
    mapping::g_mismatch_from_calls = 0;
    UEXPECT_THROW(
        cluster.Execute(ClusterHostType::kMaster, "SELECT 'not-an-integer'::text")
            .AsSingleRow<mapping::MismatchOutput>(),
        ResultSetError
    );
    EXPECT_EQ(mapping::g_mismatch_from_calls, 0);
    UEXPECT_THROW(
        cluster.Execute(ClusterHostType::kMaster, "SELECT ?::integer", mapping::NativeAggregate{1, "extra"}),
        StatementError
    );
    UEXPECT_THROW(
        cluster.Execute(ClusterHostType::kMaster, "SELECT 1::integer").AsSingleRow<mapping::Record>(),
        ResultSetError
    );
}

UTEST(Cursor, FetchesOwningTypedChunksAndMoves) {
    auto cluster = MakeCluster();

    /// [ODBC cursor]
    auto cursor = cluster.ExecuteCursor(
        ClusterHostType::kMaster,
        "SELECT value, 'row-' || value::text FROM generate_series(?::integer, ?::integer) AS value ORDER BY value",
        1,
        5
    );
    auto first = cursor.Fetch(2);
    auto moved = std::move(cursor);
    /// [ODBC cursor]

    EXPECT_TRUE(cursor.Done());
    EXPECT_EQ(cursor.FetchedSoFar(), 0);
    UEXPECT_THROW(cursor.Fetch(1), LogicError);

    struct Item final {
        std::int32_t value;
        std::string label;
    };
    const auto first_items = first.AsContainer<std::vector<Item>>();
    ASSERT_EQ(first_items.size(), 2);
    EXPECT_EQ(first_items[0].value, 1);
    EXPECT_EQ(first_items[1].label, "row-2");

    const auto second = moved.Fetch(2);
    ASSERT_EQ(second.Size(), 2);
    EXPECT_EQ(first[0][0].GetInt32(), 1);
    const auto third = moved.Fetch(2);
    ASSERT_EQ(third.Size(), 1);
    EXPECT_TRUE(moved.Done());
    EXPECT_EQ(moved.FetchedSoFar(), 5);
    UEXPECT_THROW(moved.Fetch(1), LogicError);
}

UTEST(Cursor, ExactBoundaryRequiresAnEmptyFetchToObserveEof) {
    auto cluster = MakeCluster();
    auto cursor = cluster.ExecuteCursor(ClusterHostType::kMaster, "SELECT generate_series(1, 4)");

    EXPECT_EQ(cursor.Fetch(2).Size(), 2);
    EXPECT_FALSE(cursor.Done());
    EXPECT_EQ(cursor.Fetch(2).Size(), 2);
    EXPECT_FALSE(cursor.Done());
    EXPECT_TRUE(cursor.Fetch(2).IsEmpty());
    EXPECT_TRUE(cursor.Done());
    UEXPECT_THROW(cursor.Fetch(1), LogicError);
}

UTEST(Cursor, OwnsVariadicAndParameterStoreBuffersUntilClose) {
    auto cluster = MakeCluster();
    {
        auto variadic = cluster.ExecuteCursor(
            ClusterHostType::kMaster,
            "SELECT ?::text || value::text FROM generate_series(1, 2) AS value ORDER BY value",
            std::string{"temporary-"}
        );
        EXPECT_EQ(variadic.Fetch(1)[0][0].GetString(), "temporary-1");
    }

    ParameterStore parameters;
    std::string source = "stored-";
    parameters.PushBack(source);
    auto stored = cluster.ExecuteCursor(
        ClusterHostType::kMaster,
        "SELECT ?::text || value::text FROM generate_series(1, 2) AS value ORDER BY value",
        parameters
    );
    source = "changed";
    EXPECT_EQ(stored.Fetch(2)[0][0].GetString(), "stored-1");
}

UTEST(Cursor, PreservesTypedBindingAndInjectionSafetyParity) {
    auto cluster = MakeCluster();
    const std::string hostile = "Robert'); DROP TABLE users;--";
    const std::optional<std::int32_t> null_integer;
    const Bytes bytes{{0, 1, 0, 255}};
    const Decimal<9, 4> decimal{"123.4500"};
    const Date date{2024, 2, 29};
    const Time time{23, 58, 57};
    const Timestamp timestamp{2024, 2, 29, 23, 58, 57, 123'456'000};

    auto cursor = cluster.ExecuteCursor(
        ClusterHostType::kMaster,
        "SELECT ?::text, ?::integer, ?::bytea, ?::numeric(9,4), ?::date, ?::time, ?::timestamp",
        hostile,
        null_integer,
        bytes,
        decimal,
        date,
        time,
        timestamp
    );
    const auto chunk = cursor.Fetch(2);
    ASSERT_EQ(chunk.Size(), 1);
    EXPECT_EQ(chunk[0][0].As<std::string>(), hostile);
    EXPECT_TRUE(chunk[0][1].IsNull());
    EXPECT_EQ(chunk[0][2].As<Bytes>(), bytes);
    EXPECT_EQ((chunk[0][3].As<Decimal<9, 4>>()), decimal);
    EXPECT_EQ(chunk[0][4].As<Date>(), date);
    EXPECT_EQ(chunk[0][5].As<Time>(), time);
    EXPECT_EQ(chunk[0][6].As<Timestamp>(), timestamp);
    EXPECT_TRUE(cursor.Done());
}

UTEST(Cursor, RejectsInvalidCountsAndDmlBeforeSideEffects) {
    auto cluster = MakeCluster();
    auto transaction = cluster.Begin(ClusterHostType::kMaster);
    transaction.Execute("CREATE TEMP TABLE odbc_cursor_dml(value INTEGER)");

    auto cursor = transaction.ExecuteCursor("SELECT 1");
    UEXPECT_THROW(cursor.Fetch(0), LogicError);
    EXPECT_FALSE(cursor.Done());
    EXPECT_EQ(cursor.Fetch(1).Size(), 1);
    EXPECT_TRUE(cursor.Fetch(1).IsEmpty());
    EXPECT_TRUE(cursor.Done());

    UEXPECT_THROW(transaction.ExecuteCursor("INSERT INTO odbc_cursor_dml VALUES (1)"), StatementError);
    const auto count = transaction.Execute("SELECT COUNT(*) FROM odbc_cursor_dml");
    EXPECT_EQ(count[0][0].GetInt64(), 0);
    transaction.Rollback();
}

UTEST(Cursor, PinsAndReleasesSingleConnectionPool) {
    using namespace std::chrono_literals;
    const auto host = settings::HostSettings{kDSN, {.min_size = 1, .max_size = 1}};
    Cluster cluster{settings::ODBCClusterSettings{{host}}, nullptr};

    {
        auto cursor = cluster.ExecuteCursor(ClusterHostType::kMaster, "SELECT generate_series(1, 3)");
        EXPECT_EQ(cursor.Fetch(1).Size(), 1);
        UEXPECT_THROW(
            cluster.Execute(
                ClusterHostType::kMaster,
                CommandControl{.network_timeout = 20ms, .statement_timeout = 1s},
                "SELECT 1"
            ),
            OperationInterrupted
        );
    }

    EXPECT_EQ(cluster.Execute(ClusterHostType::kMaster, "SELECT 1")[0][0].GetInt32(), 1);

    auto completed = cluster.ExecuteCursor(ClusterHostType::kMaster, "SELECT generate_series(1, 3)");
    EXPECT_EQ(completed.Fetch(8).Size(), 3);
    ASSERT_TRUE(completed.Done());
    // EOF returns the connection immediately; the Cursor object may remain alive.
    EXPECT_EQ(cluster.Execute(ClusterHostType::kMaster, "SELECT 2")[0][0].GetInt32(), 2);
}

UTEST(Cursor, TransactionGuardsOperationsAndInvalidatesOutlivingCursor) {
    auto cluster = MakeCluster();

    {
        auto transaction = cluster.Begin(ClusterHostType::kMaster);
        auto cursor = transaction.ExecuteCursor("SELECT generate_series(1, 2)");
        UEXPECT_THROW(transaction.Execute("SELECT 1"), TransactionException);
        UEXPECT_THROW(transaction.ExecuteCursor("SELECT 1"), TransactionException);
        UEXPECT_THROW(
            transaction.ExecuteBulk("INSERT INTO unused VALUES (?)", BulkParameterStore{}),
            TransactionException
        );
        UEXPECT_THROW(transaction.Commit(), TransactionException);
        UEXPECT_THROW(transaction.Rollback(), TransactionException);

        EXPECT_EQ(cursor.Fetch(4).Size(), 2);
        EXPECT_TRUE(cursor.Done());
        EXPECT_EQ(transaction.Execute("SELECT 1")[0][0].GetInt32(), 1);
        transaction.Commit();
    }

    {
        auto transaction = cluster.Begin(ClusterHostType::kMaster);
        {
            auto partial = transaction.ExecuteCursor("SELECT generate_series(1, 3)");
            EXPECT_EQ(partial.Fetch(1).Size(), 1);
        }
        EXPECT_EQ(transaction.Execute("SELECT 2")[0][0].GetInt32(), 2);
        transaction.Commit();
    }

    std::optional<Cursor> outliving;
    {
        auto transaction = cluster.Begin(ClusterHostType::kMaster);
        outliving.emplace(transaction.ExecuteCursor("SELECT generate_series(1, 3)"));
        EXPECT_EQ(outliving->Fetch(1).Size(), 1);
    }
    EXPECT_TRUE(outliving->Done());
    UEXPECT_THROW(outliving->Fetch(1), LogicError);
    EXPECT_EQ(cluster.Execute(ClusterHostType::kMaster, "SELECT 1")[0][0].GetInt32(), 1);
}

UTEST(Cursor, ConversionFailureDoesNotInvalidateOwningChunks) {
    auto cluster = MakeCluster();
    auto cursor = cluster.ExecuteCursor(ClusterHostType::kMaster, "SELECT generate_series(1, 2)::text");
    const auto first = cursor.Fetch(1);
    UEXPECT_THROW(first.AsContainer<std::vector<std::int32_t>>(), ResultSetError);
    EXPECT_EQ(first[0][0].GetString(), "1");
    EXPECT_EQ(cursor.Fetch(2)[0][0].GetString(), "2");
    EXPECT_TRUE(cursor.Done());
}

UTEST(Cursor, FetchTimeoutBreaksConnectionAndPoolRecovers) {
    using namespace std::chrono_literals;
    const std::string streaming_dsn = std::string{kDSN} + "UseDeclareFetch=1;Fetch=1;";
    const auto host = settings::HostSettings{streaming_dsn, {.min_size = 1, .max_size = 1}};
    Cluster cluster{settings::ODBCClusterSettings{{host}}, nullptr};

    auto cursor = cluster.ExecuteCursor(
        ClusterHostType::kMaster,
        CommandControl{.network_timeout = 1s, .statement_timeout = 20ms},
        "SELECT value, pg_sleep(CASE WHEN value = 1 THEN 0 ELSE 0.2 END) "
        "FROM generate_series(1, 2) AS value"
    );
    EXPECT_EQ(cursor.Fetch(1).Size(), 1);
    UEXPECT_THROW(cursor.Fetch(1), OperationInterrupted);
    EXPECT_TRUE(cursor.Done());
    EXPECT_EQ(cluster.Execute(ClusterHostType::kMaster, "SELECT 1")[0][0].GetInt32(), 1);
}

UTEST(CreateConnection, Works) { auto cluster = MakeCluster(); }

UTEST(CreateConnection, MultipleDSN) { auto cluster = MakeCluster(kMultiDSNSettings); }

UTEST(DriverCapabilities, CapturesPsqlOdbcSnapshot) {
    Connection connection{kDSN};
    const auto& capabilities = connection.GetDriverCapabilities();

    EXPECT_EQ(capabilities.GetDbmsName(), "PostgreSQL");
    EXPECT_FALSE(capabilities.GetDbmsVersion().empty());
    EXPECT_NE(capabilities.GetDriverName().find("psqlodbc"), std::string::npos);
    EXPECT_FALSE(capabilities.GetDriverVersion().empty());
    EXPECT_FALSE(capabilities.GetDriverOdbcVersion().empty());

    const auto transaction_capability = capabilities.GetTransactionCapability();
    ASSERT_TRUE(transaction_capability);
    EXPECT_NE(*transaction_capability, detail::TransactionCapability::kNone);

    const auto isolation_options = capabilities.GetTransactionIsolationOptions();
    ASSERT_TRUE(isolation_options);
    EXPECT_NE(*isolation_options & SQL_TXN_READ_COMMITTED, 0U);
    const auto default_isolation = capabilities.GetDefaultTransactionIsolation();
    ASSERT_TRUE(default_isolation);
    EXPECT_NE(*default_isolation, 0U);
    EXPECT_EQ(*default_isolation & *isolation_options, *default_isolation);

    ASSERT_TRUE(capabilities.IsDataSourceReadOnly());
    EXPECT_FALSE(*capabilities.IsDataSourceReadOnly());
    ASSERT_TRUE(capabilities.CanDescribeParameters());
    EXPECT_FALSE(*capabilities.CanDescribeParameters());

    EXPECT_TRUE(capabilities.GetParameterArrayRowCounts());
    EXPECT_TRUE(capabilities.GetParameterArraySelects());
    EXPECT_TRUE(capabilities.GetBatchRowCount());

    const auto scroll_options = capabilities.GetScrollOptions();
    ASSERT_TRUE(scroll_options);
    EXPECT_NE(*scroll_options & SQL_SO_FORWARD_ONLY, 0U);
    EXPECT_TRUE(capabilities.GetGetDataExtensions());
    EXPECT_TRUE(capabilities.GetCursorCommitBehavior());
    EXPECT_TRUE(capabilities.GetCursorRollbackBehavior());
}

UTEST(Query, Works) {
    auto cluster = MakeCluster();

    auto result = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1");
    EXPECT_EQ(result.Size(), 1);
    EXPECT_FALSE(result.IsEmpty());
    auto row = result[0];
    EXPECT_EQ(row.Size(), 1);
    EXPECT_FALSE(row[0].IsNull());
    EXPECT_EQ(row[0].GetInt32(), 1);

    auto multipleRows = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT generate_series(1, 10)");
    EXPECT_EQ(multipleRows.Size(), 10);
    for (std::size_t i = 0; i < multipleRows.Size(); i++) {
        auto row = multipleRows[i];
        EXPECT_EQ(row.Size(), 1);
        EXPECT_FALSE(row[0].IsNull());
        EXPECT_EQ(row[0].GetInt32(), i + 1);
    }
}

UTEST(Query, BindsParametersWithoutInterpolation) {
    auto cluster = MakeCluster();

    /// [ODBC parameter binding]
    const std::string untrusted_value = "Robert'); DROP TABLE users;--";
    const auto result = cluster.Execute(
        storages::odbc::ClusterHostType::kMaster,
        "SELECT ?::text, ?::text, ?::bigint, ?::bigint, ?::double precision, ?::boolean, ?::boolean",
        untrusted_value,
        std::string_view{""},
        std::int16_t{-42},
        std::uint32_t{42},
        1.25F,
        true,
        false
    );
    /// [ODBC parameter binding]

    ASSERT_EQ(result.Size(), 1);
    EXPECT_EQ(result[0][0].GetString(), untrusted_value);
    EXPECT_EQ(result[0][1].GetString(), "");
    EXPECT_EQ(result[0][2].GetInt64(), -42);
    EXPECT_EQ(result[0][3].GetInt64(), 42);
    EXPECT_DOUBLE_EQ(result[0][4].GetDouble(), 1.25);
    EXPECT_TRUE(result[0][5].GetBool());
    EXPECT_FALSE(result[0][6].GetBool());
}

UTEST(ParameterStore, OwnsDynamicParametersAndIsReusable) {
    auto cluster = MakeCluster();

    ParameterStore empty;
    EXPECT_TRUE(empty.IsEmpty());
    EXPECT_EQ(empty.Size(), 0);
    const auto empty_result = cluster.Execute(ClusterHostType::kMaster, "SELECT 1", empty);
    ASSERT_EQ(empty_result.Size(), 1);
    EXPECT_EQ(empty_result[0][0].GetInt32(), 1);

    /// [ODBC dynamic parameter store]
    const std::string injection_payload = "Robert'); DROP TABLE users;--";
    std::string copied_source = injection_payload;
    const std::optional<std::int32_t> null_integer;
    const std::optional<std::string> null_string;

    ParameterStore parameters;
    parameters.PushBack(std::int64_t{42})
        .PushBack(copied_source)
        .PushBack("string literal")
        .PushBack(std::string{"temporary string"})
        .PushBack(null_integer)
        .PushBack(null_string);

    const Query query{
        "SELECT ?::bigint, ?::text, ?::text, ?::text, ?::integer IS NULL, ?::text IS NULL",
    };
    const auto result = cluster.Execute(ClusterHostType::kMaster, query, parameters);
    /// [ODBC dynamic parameter store]

    copied_source.assign("changed after PushBack");
    EXPECT_FALSE(parameters.IsEmpty());
    EXPECT_EQ(parameters.Size(), 6);

    ParameterStore moved_parameters{std::move(parameters)};
    EXPECT_EQ(moved_parameters.Size(), 6);

    const auto validate = [&injection_payload](const ResultSet& value) {
        ASSERT_EQ(value.Size(), 1);
        EXPECT_EQ(value[0][0].GetInt64(), 42);
        EXPECT_EQ(value[0][1].GetString(), injection_payload);
        EXPECT_EQ(value[0][2].GetString(), "string literal");
        EXPECT_EQ(value[0][3].GetString(), "temporary string");
        EXPECT_TRUE(value[0][4].GetBool());
        EXPECT_TRUE(value[0][5].GetBool());
    };
    validate(result);

    // Reusing a store never consumes or mutates its values.
    validate(cluster.Execute(ClusterHostType::kMaster, CommandControl{}, query, moved_parameters));

    auto transaction = cluster.Begin(ClusterHostType::kMaster);
    validate(transaction.Execute(query, moved_parameters));
    validate(transaction.Execute(CommandControl{}, query, moved_parameters));
    UEXPECT_THROW(transaction.Execute("SELECT ?::integer", moved_parameters), StatementError);
    transaction.Rollback();

    // Unlike raw nullptr, a null const char* retains the string parameter type.
    const char* null_c_string = nullptr;
    ParameterStore null_string_parameter;
    null_string_parameter.PushBack(null_c_string);
    const auto null_result = cluster.Execute(ClusterHostType::kMaster, "SELECT ?::text IS NULL", null_string_parameter);
    ASSERT_EQ(null_result.Size(), 1);
    EXPECT_TRUE(null_result[0][0].GetBool());
}

UTEST(ParameterStore, PreservesUnsignedBigintRangeChecks) {
    auto cluster = MakeCluster();

    ParameterStore supported;
    supported.PushBack(static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()));
    const auto result = cluster.Execute(ClusterHostType::kMaster, "SELECT ?::bigint", supported);
    ASSERT_EQ(result.Size(), 1);
    EXPECT_EQ(result[0][0].GetInt64(), std::numeric_limits<std::int64_t>::max());

    ParameterStore unsupported;
    unsupported.PushBack(std::numeric_limits<std::uint64_t>::max());
    UEXPECT_THROW(cluster.Execute(ClusterHostType::kMaster, "SELECT ?::numeric", unsupported), StatementError);
}

UTEST(Query, RejectsUnsignedValuesOutsidePortableBigintRange) {
    auto cluster = MakeCluster();

    const auto supported = cluster.Execute(
        storages::odbc::ClusterHostType::kMaster,
        "SELECT ?::bigint",
        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())
    );
    ASSERT_EQ(supported.Size(), 1);
    EXPECT_EQ(supported[0][0].GetInt64(), std::numeric_limits<std::int64_t>::max());

    UEXPECT_THROW(
        cluster.Execute(
            storages::odbc::ClusterHostType::kMaster,
            "SELECT ?::numeric",
            std::numeric_limits<std::uint64_t>::max()
        ),
        storages::odbc::StatementError
    );
}

UTEST(Query, HonorsQueryLogModeAndName) {
    const Query named{
        "SELECT 'must not leak'",
        Query::Name{"safe_query_name"},
        Query::LogMode::kNameOnly,
    };
    const auto named_tags = detail::tracing::MakeQuerySpanTags(named);
    ASSERT_TRUE(named_tags.statement_name);
    EXPECT_EQ(*named_tags.statement_name, "safe_query_name");
    EXPECT_FALSE(named_tags.statement);

    const Query unnamed_name_only{
        "SELECT 'must not leak either'",
        std::nullopt,
        Query::LogMode::kNameOnly,
    };
    const auto hidden_tags = detail::tracing::MakeQuerySpanTags(unnamed_name_only);
    EXPECT_FALSE(hidden_tags.statement_name);
    EXPECT_FALSE(hidden_tags.statement);

    const Query unnamed_full{"SELECT 1"};
    const auto full_tags = detail::tracing::MakeQuerySpanTags(unnamed_full);
    EXPECT_FALSE(full_tags.statement_name);
    ASSERT_TRUE(full_tags.statement);
    EXPECT_EQ(*full_tags.statement, "SELECT 1");
}

UTEST(Query, BindsTypedNull) {
    auto cluster = MakeCluster();

    const std::optional<std::string> value;
    const auto result = cluster.Execute(
        storages::odbc::ClusterHostType::kMaster,
        "SELECT ?::text IS NULL, ?::text IS NULL",
        value,
        nullptr
    );

    ASSERT_EQ(result.Size(), 1);
    EXPECT_TRUE(result[0][0].GetBool());
    EXPECT_TRUE(result[0][1].GetBool());
}

UTEST(Query, BindsAndReadsPortableStandardTypes) {
    /// [ODBC portable standard types]
    auto cluster = MakeCluster();
    const Bytes bytes{{0, 1, 2, 0, 255}};
    const Date date{2024, 2, 29};
    const Time time{23, 58, 57};
    const Timestamp timestamp{2024, 2, 29, 23, 58, 57, 123'456'000};
    const Decimal<9, 4> decimal{"+00123.4500"};

    const auto bytes_result = cluster.Execute(ClusterHostType::kMaster, "SELECT ?::bytea", bytes);
    const auto date_result = cluster.Execute(ClusterHostType::kMaster, "SELECT ?::date", date);
    const auto time_result = cluster.Execute(ClusterHostType::kMaster, "SELECT ?::time", time);
    const auto timestamp_result = cluster.Execute(ClusterHostType::kMaster, "SELECT ?::timestamp", timestamp);
    const auto decimal_result = cluster.Execute(ClusterHostType::kMaster, "SELECT ?::numeric(9,4)", decimal);
    /// [ODBC portable standard types]

    ASSERT_EQ(bytes_result.Size(), 1);
    EXPECT_EQ(bytes_result[0][0].As<Bytes>(), bytes);
    EXPECT_EQ(bytes_result[0][0].GetString().size(), bytes.Size());

    ASSERT_EQ(date_result.Size(), 1);
    EXPECT_EQ(date_result[0][0].As<Date>(), date);
    EXPECT_EQ(date_result[0][0].GetString(), "2024-02-29");

    ASSERT_EQ(time_result.Size(), 1);
    EXPECT_EQ(time_result[0][0].As<Time>(), time);
    EXPECT_EQ(time_result[0][0].GetString(), "23:58:57");

    ASSERT_EQ(timestamp_result.Size(), 1);
    EXPECT_EQ(timestamp_result[0][0].As<Timestamp>(), timestamp);
    EXPECT_EQ(timestamp_result[0][0].GetString(), "2024-02-29 23:58:57.123456000");

    ASSERT_EQ(decimal_result.Size(), 1);
    EXPECT_EQ((decimal_result[0][0].As<Decimal<9, 4>>()), decimal);
    EXPECT_EQ(decimal_result[0][0].GetString(), "123.4500");
}

UTEST(ParameterStore, BindsPortableStandardTypesAndTypedNulls) {
    auto cluster = MakeCluster();
    const Bytes bytes{{7, 0, 8}};
    const Date date{2000, 1, 2};
    const Time time{3, 4, 5};
    const Timestamp timestamp{2000, 1, 2, 3, 4, 5, 987'654'000};
    const Decimal<9, 4> decimal{"-12.3400"};

    ParameterStore bytes_parameters;
    bytes_parameters.PushBack(bytes).PushBack(std::optional<Bytes>{});
    const auto
        bytes_result = cluster.Execute(ClusterHostType::kMaster, "SELECT ?::bytea, ?::bytea IS NULL", bytes_parameters);
    ASSERT_EQ(bytes_result.Size(), 1);
    EXPECT_EQ(bytes_result[0][0].As<Bytes>(), bytes);
    EXPECT_TRUE(bytes_result[0][1].GetBool());

    ParameterStore date_parameters;
    date_parameters.PushBack(date).PushBack(std::optional<Date>{});
    const auto
        date_result = cluster.Execute(ClusterHostType::kMaster, "SELECT ?::date, ?::date IS NULL", date_parameters);
    ASSERT_EQ(date_result.Size(), 1);
    EXPECT_EQ(date_result[0][0].As<Date>(), date);
    EXPECT_TRUE(date_result[0][1].GetBool());

    ParameterStore time_parameters;
    time_parameters.PushBack(time).PushBack(std::optional<Time>{});
    const auto
        time_result = cluster.Execute(ClusterHostType::kMaster, "SELECT ?::time, ?::time IS NULL", time_parameters);
    ASSERT_EQ(time_result.Size(), 1);
    EXPECT_EQ(time_result[0][0].As<Time>(), time);
    EXPECT_TRUE(time_result[0][1].GetBool());

    ParameterStore timestamp_parameters;
    timestamp_parameters.PushBack(timestamp).PushBack(std::optional<Timestamp>{});
    const auto timestamp_result =
        cluster.Execute(ClusterHostType::kMaster, "SELECT ?::timestamp, ?::timestamp IS NULL", timestamp_parameters);
    ASSERT_EQ(timestamp_result.Size(), 1);
    EXPECT_EQ(timestamp_result[0][0].As<Timestamp>(), timestamp);
    EXPECT_TRUE(timestamp_result[0][1].GetBool());

    ParameterStore decimal_parameters;
    decimal_parameters.PushBack(decimal).PushBack(std::optional<Decimal<9, 4>>{});
    const auto decimal_result =
        cluster
            .Execute(ClusterHostType::kMaster, "SELECT ?::numeric(9,4), ?::numeric(9,4) IS NULL", decimal_parameters);
    ASSERT_EQ(decimal_result.Size(), 1);
    EXPECT_EQ((decimal_result[0][0].As<Decimal<9, 4>>()), decimal);
    EXPECT_TRUE(decimal_result[0][1].GetBool());
}

UTEST(Query, MaterializesBinaryChunkBoundariesWithoutTerminatorHeuristics) {
    auto cluster = MakeCluster();
    const auto make_bytes = [](std::size_t size, std::uint8_t seed) {
        Bytes::Container value(size);
        for (std::size_t index = 0; index < value.size(); ++index) {
            value[index] = static_cast<std::uint8_t>(seed + index);
        }
        return Bytes{std::move(value)};
    };
    const Bytes empty;
    const auto bytes_4095 = make_bytes(4095, 1);
    const auto bytes_4096 = make_bytes(4096, 2);
    const auto bytes_4097 = make_bytes(4097, 3);
    const auto bytes_long = make_bytes(65'537, 4);

    const auto result = cluster.Execute(
        ClusterHostType::kMaster,
        "SELECT ?::bytea, ?::bytea, ?::bytea, ?::bytea, ?::bytea",
        empty,
        bytes_4095,
        bytes_4096,
        bytes_4097,
        bytes_long
    );

    ASSERT_EQ(result.Size(), 1);
    EXPECT_EQ(result[0][0].As<Bytes>(), empty);
    EXPECT_EQ(result[0][1].As<Bytes>(), bytes_4095);
    EXPECT_EQ(result[0][2].As<Bytes>(), bytes_4096);
    EXPECT_EQ(result[0][3].As<Bytes>(), bytes_4097);
    EXPECT_EQ(result[0][4].As<Bytes>(), bytes_long);
}

UTEST(Query, ParameterCountMismatchIsStatementError) {
    auto cluster = MakeCluster();
    UEXPECT_THROW(
        cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT ?::integer", 1, 2),
        storages::odbc::StatementError
    );
}

UTEST(Query, VariousTypes) {
    auto query = "SELECT 42, 'test', 1.0, false, null, true";
    auto cluster = MakeCluster();

    auto result = cluster.Execute(storages::odbc::ClusterHostType::kMaster, query);
    EXPECT_EQ(result.Size(), 1);
    EXPECT_FALSE(result.IsEmpty());
    auto row = result[0];
    EXPECT_EQ(row.Size(), 6);

    auto intField = row[0];
    EXPECT_EQ(intField.GetInt32(), 42);

    auto stringField = row[1];
    EXPECT_EQ(stringField.GetString(), "test");

    auto doubleField = row[2];
    EXPECT_DOUBLE_EQ(doubleField.GetDouble(), 1.0);

    auto boolField = row[3];
    EXPECT_EQ(boolField.GetBool(), false);

    auto nullField = row[4];
    EXPECT_TRUE(nullField.IsNull());

    auto trueBool = row[5];
    EXPECT_EQ(trueBool.GetBool(), true);
}

UTEST(Query, DifferentHostTypes) {
    auto query = "SELECT 1";
    auto cluster = MakeCluster();

    // TODO: needs an actual check that host are selected correctly
    cluster.Execute(storages::odbc::ClusterHostType::kMaster, query);
    cluster.Execute(storages::odbc::ClusterHostType::kSlave, query);
    cluster.Execute(storages::odbc::ClusterHostType::kNone, query);
}

UTEST(Query, EmptyResult) {
    auto cluster = MakeCluster();
    auto result = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1 WHERE false");
    EXPECT_EQ(result.Size(), 0);
    EXPECT_TRUE(result.IsEmpty());
}

UTEST(Query, FieldCount) {
    auto cluster = MakeCluster();
    auto result = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1, 2, 3");
    EXPECT_EQ(result.FieldCount(), 3);
    EXPECT_EQ(result.Size(), 1);
    EXPECT_EQ(result[0].Size(), 3);
}

UTEST(Query, GetInt64) {
    auto cluster = MakeCluster();
    auto result = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 2147483648");
    EXPECT_EQ(result[0][0].GetInt64(), 2147483648LL);
}

UTEST(Query, GetStringFromNumber) {
    auto cluster = MakeCluster();
    auto result = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT '42'");
    EXPECT_EQ(result[0][0].GetString(), "42");
}

UTEST(Query, MaterializedResultOutlivesConnectionAndSubsequentQuery) {
    const auto host_settings = storages::odbc::settings::HostSettings{kDSN, {1, 1}};
    std::optional<ResultSet> saved_result;
    {
        storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{host_settings}}, nullptr);
        saved_result
            .emplace(cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT ?::text AS saved_value", "first")
            );

        const auto second = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT ?::integer", 2);
        ASSERT_EQ(second.Size(), 1);
        EXPECT_EQ(second[0][0].GetInt32(), 2);
    }

    ASSERT_TRUE(saved_result);
    ASSERT_EQ(saved_result->Size(), 1);
    EXPECT_EQ(saved_result->GetFieldName(0), "saved_value");
    EXPECT_EQ((*saved_result)[0][0].GetString(), "first");
}

UTEST(Query, MaterializesLongEmptyAndNullValues) {
    auto cluster = MakeCluster();
    const std::string long_value(70'000, 'x');

    const auto result = cluster.Execute(
        storages::odbc::ClusterHostType::kMaster,
        "SELECT ?::text AS long_value, ?::text AS empty_value, NULL::text AS null_value",
        long_value,
        std::string{}
    );

    ASSERT_EQ(result.Size(), 1);
    EXPECT_EQ(result[0][0].GetString(), long_value);
    EXPECT_EQ(result[0][1].GetString(), "");
    EXPECT_TRUE(result[0][2].IsNull());
    UEXPECT_THROW(result[0][2].GetString(), storages::odbc::ResultSetError);
}

UTEST(Query, MaterializesChunkBoundariesAndTypedNulls) {
    auto cluster = MakeCluster();
    const std::string value_4095(4095, 'a');
    const std::string value_4096(4096, 'b');
    const std::string value_4097(4097, 'c');
    const std::string value_65537(65'537, 'd');

    const auto result = cluster.Execute(
        storages::odbc::ClusterHostType::kMaster,
        "SELECT ?::text, ?::text, ?::text, ?::text, NULL::integer, NULL::boolean, NULL::double precision",
        value_4095,
        value_4096,
        value_4097,
        value_65537
    );

    ASSERT_EQ(result.Size(), 1);
    EXPECT_EQ(result[0][0].GetString(), value_4095);
    EXPECT_EQ(result[0][1].GetString(), value_4096);
    EXPECT_EQ(result[0][2].GetString(), value_4097);
    EXPECT_EQ(result[0][3].GetString(), value_65537);
    for (std::size_t index = 4; index < 7; ++index) {
        EXPECT_TRUE(result[0][index].IsNull());
    }
    UEXPECT_THROW(result[0][4].GetInt32(), storages::odbc::ResultSetError);
    UEXPECT_THROW(result[0][5].GetBool(), storages::odbc::ResultSetError);
    UEXPECT_THROW(result[0][6].GetDouble(), storages::odbc::ResultSetError);
}

UTEST(Query, MaterializedResultSurvivesTopologyReload) {
    const auto host_settings = storages::odbc::settings::HostSettings{kDSN, {1, 1}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{host_settings}}, nullptr);
    const auto result = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT ?::text", "before reload");

    const std::string reloaded_dsn = std::string{kDSN} + "ApplicationName=odbc-materialized-result;";
    cluster.UpdateSettings(storages::odbc::settings::ODBCClusterSettings{{
        storages::odbc::settings::HostSettings{reloaded_dsn, {1, 1}},
    }});

    ASSERT_EQ(result.Size(), 1);
    EXPECT_EQ(result[0][0].GetString(), "before reload");
}

UTEST(Query, SeparatesRowsFromRowsAffected) {
    const auto host_settings = storages::odbc::settings::HostSettings{kDSN, {1, 1}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{host_settings}}, nullptr);
    cluster.Execute(storages::odbc::ClusterHostType::kMaster, "CREATE TEMP TABLE odbc_rows_affected(value INTEGER)");

    const auto insert = cluster.Execute(
        storages::odbc::ClusterHostType::kMaster,
        "INSERT INTO odbc_rows_affected(value) VALUES (?), (?)",
        1,
        2
    );
    EXPECT_EQ(insert.Size(), 0);
    EXPECT_EQ(insert.RowsAffected(), 2);

    const auto select =
        cluster
            .Execute(storages::odbc::ClusterHostType::kMaster, "SELECT value FROM odbc_rows_affected ORDER BY value");
    EXPECT_EQ(select.Size(), 2);
    EXPECT_EQ(select.RowsAffected(), 0);
}

UTEST(Pool, LessQueriesThanConnections) {
    std::size_t poolConnections = 5;
    auto hostSettings = storages::odbc::settings::HostSettings{kDSN, {poolConnections, poolConnections}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{hostSettings}}, nullptr);

    std::vector<engine::TaskWithResult<ResultSet>> futures;
    futures.reserve(poolConnections);

    for (std::size_t i = 0; i < poolConnections - 1; i++) {
        futures.emplace_back(utils::Async("LessQueriesThanConnections", [&cluster]() {
            return cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1");
        }));
    }

    for (auto& future : futures) {
        auto result = future.Get();
        EXPECT_EQ(result.Size(), 1);
        EXPECT_EQ(result[0][0].GetInt32(), 1);
    }
}

UTEST(Pool, EqualQueriesAndConnections) {
    std::size_t poolConnections = 5;
    auto hostSettings = storages::odbc::settings::HostSettings{kDSN, {poolConnections, poolConnections}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{hostSettings}}, nullptr);

    std::vector<engine::TaskWithResult<ResultSet>> futures;
    futures.reserve(poolConnections);

    for (std::size_t i = 0; i < poolConnections; i++) {
        futures.emplace_back(utils::Async("EqualQueriesAndConnections", [&cluster]() {
            return cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1");
        }));
    }

    for (auto& future : futures) {
        auto result = future.Get();
        EXPECT_EQ(result.Size(), 1);
        EXPECT_EQ(result[0][0].GetInt32(), 1);
    }
}

UTEST(Pool, MoreQueriesThanConnectionsButLessThanPoolSize) {
    std::size_t poolConnections = 5;
    auto hostSettings = storages::odbc::settings::HostSettings{kDSN, {poolConnections, poolConnections * 2}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{hostSettings}}, nullptr);

    std::vector<engine::TaskWithResult<ResultSet>> futures;
    futures.reserve(poolConnections + 2);

    for (std::size_t i = 0; i < poolConnections + 2; i++) {
        futures.emplace_back(utils::Async("MoreQueriesThanConnectionsButLessThanPoolSize", [&cluster]() {
            return cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1");
        }));
    }

    for (auto& future : futures) {
        auto result = future.Get();
        EXPECT_EQ(result.Size(), 1);
        EXPECT_EQ(result[0][0].GetInt32(), 1);
    }
}

UTEST(Pool, MoreQueriesThanConnectionsAndPoolSize) {
    std::size_t poolConnections = 5;
    auto hostSettings = storages::odbc::settings::HostSettings{kDSN, {poolConnections, poolConnections}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{hostSettings}}, nullptr);

    std::vector<engine::TaskWithResult<ResultSet>> futures;
    futures.reserve(poolConnections * 2);

    for (std::size_t i = 0; i < poolConnections * 2; i++) {
        futures.emplace_back(utils::Async("MoreQueriesThanConnectionsAndPoolSize", [&cluster]() {
            return cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1");
        }));
    }

    for (auto& future : futures) {
        auto result = future.Get();
        EXPECT_EQ(result.Size(), 1);
        EXPECT_EQ(result[0][0].GetInt32(), 1);
    }
}

UTEST(Pool, RestoresBrokenConnection) {
    auto hostSettings = storages::odbc::settings::HostSettings{kDSN, {1, 1}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{hostSettings}}, nullptr);

    auto killConnectionQuery = "SELECT pg_terminate_backend(pid) FROM pg_stat_activity WHERE datname = 'postgres';";

    try {
        cluster.Execute(storages::odbc::ClusterHostType::kMaster, killConnectionQuery);
    } catch (...) {
    }

    auto selectRes = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1");
    EXPECT_EQ(selectRes.Size(), 1);
    EXPECT_EQ(selectRes[0][0].GetInt32(), 1);
}

}  // namespace storages::odbc::tests

USERVER_NAMESPACE_END
