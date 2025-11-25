#include <gtest/gtest.h>

#include <concepts>
#include <optional>
#include <vector>

#include <fmt/format.h>

#include <userver/proto-structs/convert.hpp>
#include <userver/proto-structs/hash_map.hpp>
#include <userver/proto-structs/type_mapping.hpp>
#include <userver/utils/box.hpp>

#include <box/options/initialization.pb.h>
#include <box/options/initialization.structs.usrv.pb.hpp>
#include <test_utils/type_assertions.hpp>

namespace structs = box::options::structs;

USERVER_NAMESPACE_BEGIN

TEST(BoxInitialization, NotDefinedYet) { AssertFieldCount<structs::NotDefinedYet, 0>(); }

namespace {

template <typename FieldType, typename MessageType>
void CheckDependsOnNotDefinedYet() {
    AssertFieldCount<MessageType, 5>();
    AssertFieldType<decltype(MessageType::plain_field), utils::Box<FieldType>>();
    AssertFieldType<decltype(MessageType::optional_field), std::optional<utils::Box<FieldType>>>();
    AssertFieldType<decltype(MessageType::vector_field), std::vector<FieldType>>();
    AssertFieldType<decltype(MessageType::map_field), utils::Box<proto_structs::HashMap<std::string, FieldType>>>();
    AssertFieldType<decltype(MessageType::oneof), typename MessageType::Oneof>();

    static_assert(std::same_as<
                  proto_structs::OneofAlternativeType<0, typename MessageType::Oneof>,
                  utils::Box<FieldType>>);

    MessageType test_struct{
        .plain_field = FieldType{},
        .optional_field = FieldType{},
        .vector_field = {FieldType{}},
        .map_field = {{{"key", FieldType{}}}},
        .oneof = MessageType::Oneof::make_oneof_field(FieldType{}),
    };

    auto test_message = proto_structs::StructToMessage(test_struct);
    auto test_struct_again = proto_structs::MessageToStruct<MessageType>(test_message);

    // TODO re-enable test once operator== is available
    // ASSERT_EQ(test_struct_again, test_struct);
}

}  // namespace

TEST(BoxInitialization, DependsOnNotDefinedYet) {
    CheckDependsOnNotDefinedYet<structs::NotDefinedYet, structs::DependsOnNotDefinedYet>();
}

TEST(BoxInitialization, IndirectlyDependsOnNotDefinedYet) {
    CheckDependsOnNotDefinedYet<structs::DependsOnNotDefinedYet, structs::IndirectlyDependsOnNotDefinedYet>();
}

USERVER_NAMESPACE_END
