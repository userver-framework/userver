#include <gtest/gtest.h>

#include <enums/names.structs.usrv.pb.hpp>

#include <userver/utest/assert_macros.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using Enum = enums::structs::Unprefixed;

}  // namespace

TEST(EnumStringOperations, ToString) { ASSERT_EQ(enums::structs::ToString(Enum::kFooVar), "FOO_VAR"); }

TEST(EnumStringOperations, ToStringView) { ASSERT_EQ(enums::structs::ToStringView(Enum::kFooVar), "FOO_VAR"); }

TEST(EnumStringOperations, Parse) {
    ASSERT_EQ(enums::structs::Parse("FOO_VAR", formats::parse::To<Enum>()), Enum::kFooVar);
    UEXPECT_THROW_MSG(
        enums::structs::Parse("INVALID", formats::parse::To<Enum>()),
        proto_structs::ConversionError,
        "unknown enum value: INVALID"
    );
}

USERVER_NAMESPACE_END
