#include <gmock/gmock.h>

#include <userver/formats/json/inline.hpp>
#include <userver/formats/parse/variant.hpp>
#include <userver/utils/box.hpp>

#include <schemas/oneof_discriminator_indirect.hpp>

#include "helper.hpp"

USERVER_NAMESPACE_BEGIN

TEST(OneOfDiscriminatorIndirect, ParseSimpleVariant) {
    const auto json = formats::json::MakeObject(
        "root",
        formats::json::MakeObject("kind", 1, "value", 42)
    );
    auto tree = json.As<ns::Tree>();

    EXPECT_TRUE(std::holds_alternative<utils::Box<ns::TypeA>>(*tree.root));
    EXPECT_EQ(std::get<utils::Box<ns::TypeA>>(*tree.root)->value, 42);

    EXPECT_EQ(TestDomSerializer(tree), json);
    EXPECT_EQ(TestWriteToStream(tree), json);
}

TEST(OneOfDiscriminatorIndirect, ParseVariantWithIndirectNested) {
    const auto json = formats::json::MakeObject(
        "root",
        formats::json::MakeObject(
            "kind", 2,
            "nested", formats::json::MakeObject(
                "kind", 1,
                "value", 100
            )
        )
    );
    auto tree = json.As<ns::Tree>();
    EXPECT_TRUE(std::holds_alternative<utils::Box<ns::TypeB>>(*tree.root));

    auto& type_b = std::get<utils::Box<ns::TypeB>>(*tree.root);
    EXPECT_TRUE(type_b->nested.has_value());
    EXPECT_TRUE(std::holds_alternative<utils::Box<ns::TypeA>>(*type_b->nested.value()));
    EXPECT_EQ(std::get<utils::Box<ns::TypeA>>(*type_b->nested.value())->value, 100);


    EXPECT_EQ(TestDomSerializer(tree), json);
    EXPECT_EQ(TestWriteToStream(tree), json);
}

USERVER_NAMESPACE_END
