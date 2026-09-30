#include <storages/mysql/impl/bindings/native_binds_helper.hpp>

#include <limits>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::tests {

namespace {

constexpr std::size_t kProtocolMax = std::numeric_limits<std::uint32_t>::max();

MYSQL_FIELD MakeField(enum_field_types type, unsigned long length, unsigned long max_length) {
    MYSQL_FIELD field{};
    field.type = type;
    field.length = length;
    field.max_length = max_length;
    return field;
}

}  // namespace

UTEST(FetchedByteLength, GetMaxFromFieldMaxLength) {
    const auto field = MakeField(MYSQL_TYPE_VAR_STRING, 0, 128);
    EXPECT_EQ(impl::bindings::GetMaxFetchedByteLength(field), 128U);
}

UTEST(FetchedByteLength, GetMaxPrefersMetadataMaxLengthOverBlobType) {
    const auto field = MakeField(MYSQL_TYPE_TINY_BLOB, 0, 100);
    EXPECT_EQ(impl::bindings::GetMaxFetchedByteLength(field), 100U);
}

UTEST(FetchedByteLength, GetMaxCapsMetadataAboveProtocolLimit) {
    const auto field = MakeField(MYSQL_TYPE_VAR_STRING, 0, static_cast<unsigned long>(kProtocolMax) + 1);
    EXPECT_EQ(impl::bindings::GetMaxFetchedByteLength(field), kProtocolMax);
}

UTEST(FetchedByteLength, GetMaxFromFieldLengthWhenMaxLengthUnset) {
    const auto field = MakeField(MYSQL_TYPE_VAR_STRING, 64, 0);
    EXPECT_EQ(impl::bindings::GetMaxFetchedByteLength(field), 64U);
}

UTEST(FetchedByteLength, GetMaxFromTinyBlobType) {
    const auto field = MakeField(MYSQL_TYPE_TINY_BLOB, 0, 0);
    EXPECT_EQ(impl::bindings::GetMaxFetchedByteLength(field), 255U);
}

UTEST(FetchedByteLength, GetMaxFromBlobType) {
    const auto field = MakeField(MYSQL_TYPE_BLOB, 0, 0);
    EXPECT_EQ(impl::bindings::GetMaxFetchedByteLength(field), 65535U);
}

UTEST(FetchedByteLength, GetMaxFromMediumBlobType) {
    const auto field = MakeField(MYSQL_TYPE_MEDIUM_BLOB, 0, 0);
    EXPECT_EQ(impl::bindings::GetMaxFetchedByteLength(field), 16777215U);
}

UTEST(FetchedByteLength, GetMaxFromLongBlobType) {
    const auto field = MakeField(MYSQL_TYPE_LONG_BLOB, 0, 0);
    EXPECT_EQ(impl::bindings::GetMaxFetchedByteLength(field), kProtocolMax);
}

UTEST(FetchedByteLength, GetMaxFallbackToProtocolLimit) {
    const auto field = MakeField(MYSQL_TYPE_LONG, 0, 0);
    EXPECT_EQ(impl::bindings::GetMaxFetchedByteLength(field), kProtocolMax);
}

UTEST(FetchedByteLength, ValidateAcceptsWithinFieldLimit) {
    const auto length = impl::bindings::ValidateFetchedByteLength(100, /*field_max_byte_length=*/255);
    EXPECT_EQ(length, 100U);
}

UTEST(FetchedByteLength, ValidateAcceptsExactFieldLimit) {
    const auto length = impl::bindings::ValidateFetchedByteLength(255, /*field_max_byte_length=*/255);
    EXPECT_EQ(length, 255U);
}

UTEST(FetchedByteLength, ValidateRejectsAboveFieldLimit) {
    EXPECT_THROW(impl::bindings::ValidateFetchedByteLength(256, /*field_max_byte_length=*/255), std::runtime_error);
}

UTEST(FetchedByteLength, ValidateRejectsAboveProtocolLimit) {
    constexpr unsigned long kAboveProtocolMax = static_cast<unsigned long>(kProtocolMax) + 1;
    EXPECT_THROW(impl::bindings::ValidateFetchedByteLength(kAboveProtocolMax, kProtocolMax), std::runtime_error);
}

}  // namespace storages::mysql::tests

USERVER_NAMESPACE_END
