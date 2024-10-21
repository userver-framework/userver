#include <userver/utest/assert_macros.hpp>

#include <schemas/all_of_fwd.hpp>
#include <schemas/array_fwd.hpp>
#include <schemas/custom_cpp_type_fwd.hpp>
#include <schemas/external_ref_fwd.hpp>
#include <schemas/indirect_fwd.hpp>
#include <schemas/int_minmax_fwd.hpp>
#include <schemas/invalid_names_fwd.hpp>
#include <schemas/object_empty_fwd.hpp>
#include <schemas/object_extra_fwd.hpp>
#include <schemas/object_object_fwd.hpp>
#include <schemas/object_single_field_fwd.hpp>
#include <schemas/one_of_fwd.hpp>
#include <schemas/pattern_fwd.hpp>

USERVER_NAMESPACE_BEGIN

TEST(Fwd, DeclaredStruct) {
    [[maybe_unused]] ns::Object1* object1_ptr = nullptr;
    [[maybe_unused]] ns::ArrayStruct* array_struct_ptr = nullptr;
    [[maybe_unused]] ns::CustomStruct1* custom_struct_ptr = nullptr;
    [[maybe_unused]] ns::ObjectWithExternalRef* object_with_external_ref_ptr = nullptr;
    [[maybe_unused]] ns::TreeNode* tree_node_ptr = nullptr;
    [[maybe_unused]] ns::IntegerObject* integer_object_ptr = nullptr;
    [[maybe_unused]] ns::ObjectInvalid* object_invalid_ptr = nullptr;
    [[maybe_unused]] ns::ObjectEmpty* object_empty_ptr = nullptr;
    [[maybe_unused]] ns::ObjectExtra* object_extra_ptr = nullptr;
    [[maybe_unused]] ns::Objectx* objectx_ptr = nullptr;
    [[maybe_unused]] ns::SimpleObject* simple_ptr = nullptr;
    [[maybe_unused]] ns::ObjectOneOfWithDiscriminator* object_one_of_with_discriminator_ptr = nullptr;
    [[maybe_unused]] ns::ObjectPattern* object_pattern_ptr = nullptr;
}

TEST(Fwd, DeclaredAllOf) { [[maybe_unused]] ns::AllOf* all_of_ptr = nullptr; }

TEST(Fwd, DeclaredIntEnum) { [[maybe_unused]] ns::IntegerEnum* int_enum_ptr = nullptr; }

TEST(Fwd, DeclaredStringEnum) { [[maybe_unused]] ns::StringEnum* string_enum_ptr = nullptr; }

USERVER_NAMESPACE_END
