#include <userver/decimal64/decimal64.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/utest/utest.hpp>

#include <storages/mysql/impl/bindings/output_bindings.hpp>

#include <userver/storages/mysql/impl/io/decimal_binder.hpp>

#include "output_bindings_fetched_length_test_peer.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::tests {

namespace {

using Decimal = decimal64::Decimal<3>;

constexpr std::size_t kBindPos = 0;
constexpr unsigned long kColumnMaxBytes = 255;
constexpr unsigned long kReportedWithinLimit = 100;
constexpr unsigned long kReportedAboveLimit = 256;

struct VariableLengthOutputBindFixture final {
    std::string string{};
    std::optional<std::string> optional_string{};
    formats::json::Value json{};
    std::optional<formats::json::Value> optional_json{};
    Decimal decimal{"1.000"};
    std::optional<Decimal> optional_decimal{};
    std::optional<impl::io::DecimalWrapper> optional_decimal_wrapper{};

    void BindString(impl::bindings::OutputBindings& binds) { binds.Bind(kBindPos, string); }

    void BindOptionalString(impl::bindings::OutputBindings& binds) { binds.Bind(kBindPos, optional_string); }

    void BindJson(impl::bindings::OutputBindings& binds) { binds.Bind(kBindPos, json); }

    void BindOptionalJson(impl::bindings::OutputBindings& binds) { binds.Bind(kBindPos, optional_json); }

    void BindDecimal(impl::bindings::OutputBindings& binds) {
        impl::io::DecimalWrapper wrapper{decimal};
        binds.Bind(kBindPos, wrapper);
    }

    void BindOptionalDecimal(impl::bindings::OutputBindings& binds) {
        optional_decimal.emplace();
        optional_decimal_wrapper.emplace(*optional_decimal);
        binds.Bind(kBindPos, optional_decimal_wrapper);
    }
};

void PrepareFetch(impl::bindings::OutputBindings& binds, unsigned long reported_length, bool is_null) {
    OutputBindingsFetchedLengthTestPeer::SetColumnMaxFetchedByteLength(binds, kBindPos, kColumnMaxBytes);
    auto& bind = binds.GetBindsArray()[kBindPos];
    bind.length_value = reported_length;
    bind.is_null_value = is_null ? 1 : 0;
}

}  // namespace

UTEST(OutputBindingsFetchedLength, StringRejectsReportedLengthAboveColumnMax) {
    VariableLengthOutputBindFixture fixture{};
    impl::bindings::OutputBindings binds{1};
    fixture.BindString(binds);
    PrepareFetch(binds, kReportedAboveLimit, false);

    EXPECT_THROW(binds.BeforeFetch(kBindPos), std::runtime_error);
    EXPECT_TRUE(fixture.string.empty());
}

UTEST(OutputBindingsFetchedLength, StringAcceptsReportedLengthWithinColumnMax) {
    VariableLengthOutputBindFixture fixture{};
    impl::bindings::OutputBindings binds{1};
    fixture.BindString(binds);
    PrepareFetch(binds, kReportedWithinLimit, false);

    binds.BeforeFetch(kBindPos);

    EXPECT_EQ(fixture.string.size(), kReportedWithinLimit);
}

UTEST(OutputBindingsFetchedLength, OptionalStringAcceptsReportedLengthWithinColumnMax) {
    VariableLengthOutputBindFixture fixture{};
    impl::bindings::OutputBindings binds{1};
    fixture.BindOptionalString(binds);
    PrepareFetch(binds, kReportedWithinLimit, false);

    binds.BeforeFetch(kBindPos);

    ASSERT_TRUE(fixture.optional_string.has_value());
    EXPECT_EQ(fixture.optional_string->size(), kReportedWithinLimit);
}

UTEST(OutputBindingsFetchedLength, OptionalStringRejectsReportedLengthAboveColumnMax) {
    VariableLengthOutputBindFixture fixture{};
    impl::bindings::OutputBindings binds{1};
    fixture.BindOptionalString(binds);
    PrepareFetch(binds, kReportedAboveLimit, false);

    EXPECT_THROW(binds.BeforeFetch(kBindPos), std::runtime_error);
    EXPECT_FALSE(fixture.optional_string.has_value());
}

UTEST(OutputBindingsFetchedLength, OptionalStringSkipsFetchWhenNull) {
    VariableLengthOutputBindFixture fixture{};
    impl::bindings::OutputBindings binds{1};
    fixture.BindOptionalString(binds);
    PrepareFetch(binds, kReportedAboveLimit, true);

    binds.BeforeFetch(kBindPos);

    EXPECT_FALSE(fixture.optional_string.has_value());
}

UTEST(OutputBindingsFetchedLength, JsonAcceptsReportedLengthWithinColumnMax) {
    VariableLengthOutputBindFixture fixture{};
    impl::bindings::OutputBindings binds{1};
    fixture.BindJson(binds);
    PrepareFetch(binds, kReportedWithinLimit, false);

    binds.BeforeFetch(kBindPos);

    EXPECT_EQ(OutputBindingsFetchedLengthTestPeer::IntermediateStringSize(binds, kBindPos), kReportedWithinLimit);
}

UTEST(OutputBindingsFetchedLength, JsonRejectsReportedLengthAboveColumnMax) {
    VariableLengthOutputBindFixture fixture{};
    impl::bindings::OutputBindings binds{1};
    fixture.BindJson(binds);
    PrepareFetch(binds, kReportedAboveLimit, false);

    EXPECT_THROW(binds.BeforeFetch(kBindPos), std::runtime_error);
    EXPECT_EQ(OutputBindingsFetchedLengthTestPeer::IntermediateStringSize(binds, kBindPos), 0U);
}

UTEST(OutputBindingsFetchedLength, OptionalJsonRejectsReportedLengthAboveColumnMax) {
    VariableLengthOutputBindFixture fixture{};
    impl::bindings::OutputBindings binds{1};
    fixture.BindOptionalJson(binds);
    PrepareFetch(binds, kReportedAboveLimit, false);

    EXPECT_THROW(binds.BeforeFetch(kBindPos), std::runtime_error);
    EXPECT_FALSE(fixture.optional_json.has_value());
}

UTEST(OutputBindingsFetchedLength, OptionalJsonSkipsFetchWhenNull) {
    VariableLengthOutputBindFixture fixture{};
    impl::bindings::OutputBindings binds{1};
    fixture.BindOptionalJson(binds);
    PrepareFetch(binds, kReportedAboveLimit, true);

    binds.BeforeFetch(kBindPos);

    EXPECT_FALSE(fixture.optional_json.has_value());
}

UTEST(OutputBindingsFetchedLength, DecimalAcceptsReportedLengthWithinColumnMax) {
    VariableLengthOutputBindFixture fixture{};
    impl::bindings::OutputBindings binds{1};
    fixture.BindDecimal(binds);
    PrepareFetch(binds, kReportedWithinLimit, false);

    binds.BeforeFetch(kBindPos);

    EXPECT_EQ(OutputBindingsFetchedLengthTestPeer::IntermediateStringSize(binds, kBindPos), kReportedWithinLimit);
}

UTEST(OutputBindingsFetchedLength, DecimalRejectsReportedLengthAboveColumnMax) {
    VariableLengthOutputBindFixture fixture{};
    impl::bindings::OutputBindings binds{1};
    fixture.BindDecimal(binds);
    PrepareFetch(binds, kReportedAboveLimit, false);

    EXPECT_THROW(binds.BeforeFetch(kBindPos), std::runtime_error);
    EXPECT_EQ(OutputBindingsFetchedLengthTestPeer::IntermediateStringSize(binds, kBindPos), 0U);
}

UTEST(OutputBindingsFetchedLength, OptionalDecimalRejectsReportedLengthAboveColumnMax) {
    VariableLengthOutputBindFixture fixture{};
    impl::bindings::OutputBindings binds{1};
    fixture.BindOptionalDecimal(binds);
    PrepareFetch(binds, kReportedAboveLimit, false);

    EXPECT_THROW(binds.BeforeFetch(kBindPos), std::runtime_error);
    EXPECT_EQ(OutputBindingsFetchedLengthTestPeer::IntermediateStringSize(binds, kBindPos), 0U);
}

UTEST(OutputBindingsFetchedLength, OptionalDecimalSkipsFetchWhenNull) {
    VariableLengthOutputBindFixture fixture{};
    impl::bindings::OutputBindings binds{1};
    fixture.BindOptionalDecimal(binds);
    PrepareFetch(binds, kReportedAboveLimit, true);

    binds.BeforeFetch(kBindPos);

    EXPECT_EQ(OutputBindingsFetchedLengthTestPeer::IntermediateStringSize(binds, kBindPos), 0U);
}

}  // namespace storages::mysql::tests

USERVER_NAMESPACE_END
