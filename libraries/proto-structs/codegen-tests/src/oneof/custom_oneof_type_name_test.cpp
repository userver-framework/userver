#include <gtest/gtest.h>

#include <concepts>

#include <userver/proto-structs/type_mapping.hpp>

#include <oneof/custom_oneof_type_name.structs.usrv.pb.hpp>
#include <test_utils/type_assertions.hpp>

USERVER_NAMESPACE_BEGIN

TEST(CustomOneofTypeName, Conflict1) {
    using Scope = oneof::structs::NameConflict1;
    AssertFieldCount<Scope::Nested, 1>();
    // `FooIdCustom` type name was set by `option (userver.structs.oneof).generated_type_name`.
    // Without the option, it would be called `FooId`, and there would be a naming conflict.
    AssertFieldType<decltype(Scope::Nested::foo_id), Scope::Nested::FooIdCustom>();
    static_assert(std::same_as<proto_structs::OneofAlternativeType<0, Scope::Nested::FooIdCustom>, Scope::FooId>);
}

TEST(CustomOneofTypeName, Conflict2) {
    using Scope = oneof::structs::NameConflict2;
    AssertFieldCount<Scope::Nested, 1>();
    // `TFooIdCustom` type name was set by `option (userver.structs.oneof).generated_type_name`.
    // Without the option, it would be called `TFooId`, and there would be a naming conflict.
    AssertFieldType<decltype(Scope::Nested::FooId), Scope::Nested::TFooIdCustom>();
    static_assert(std::same_as<proto_structs::OneofAlternativeType<0, Scope::Nested::TFooIdCustom>, Scope::TFooId>);
}

USERVER_NAMESPACE_END
