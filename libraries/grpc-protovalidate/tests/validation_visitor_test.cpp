#include <userver/grpc-protovalidate/validate.hpp>

#include <string>
#include <vector>

#include <google/protobuf/any.pb.h>
#include <google/protobuf/util/message_differencer.h>

#include <userver/engine/task/task.hpp>
#include <userver/engine/wait_all_checked.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>

#include <grpc-protovalidate/impl/validation_visitor.hpp>

#include <types/constrained.pb.h>
#include <types/extensions.pb.h>

#include "utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace grpc_protovalidate {

namespace {

void PopulateIgnoredCollections(types::TraversalRoot& message) {
    message.add_ignored_items();
    message.add_ignored_items();
    (*message.mutable_ignored_values())["one"].set_value(0);
    (*message.mutable_ignored_values())["two"].set_value(0);
}

void ExpectSameValidation(const google::protobuf::Message& message) {
    auto factory = buf::validate::ValidatorFactory::New();
    ASSERT_TRUE(factory.ok());
    for (const bool fail_fast : {false, true}) {
        SCOPED_TRACE(fail_fast);
        google::protobuf::Arena arena;
        auto validator = factory.value()->NewValidator(&arena, fail_fast);
        auto expected = validator.Validate(message);
        const auto actual = ValidateMessage(message, {.fail_fast = fail_fast});
        if (!expected.ok()) {
            ASSERT_FALSE(actual.IsSuccess());
            EXPECT_EQ(actual.GetError().GetType(), ValidationError::Type::kInternal);
            EXPECT_NE(actual.GetError().GetDescription().find(expected.status().ToString()), std::string::npos);
        } else if (expected->success()) {
            EXPECT_TRUE(actual.IsSuccess());
        } else {
            ASSERT_FALSE(actual.IsSuccess());
            EXPECT_EQ(actual.GetError().GetType(), ValidationError::Type::kRule);
            buf::validate::Violations actual_proto;
            for (const auto& violation : actual.GetError().GetViolations()) {
                *actual_proto.add_violations() = violation.proto();
            }
            const auto expected_proto = expected->proto();
            EXPECT_TRUE(google::protobuf::util::MessageDifferencer::Equals(expected_proto, actual_proto)
            ) << "Expected: "
              << expected_proto.DebugString() << "Actual: " << actual_proto.DebugString();
        }
    }
}

}  // namespace

UTEST(ValidationVisitor, NestedPathsAndOrder) {
    types::TraversalRoot root;
    PopulateIgnoredCollections(root);
    root.mutable_singular();
    root.add_repeated()->set_value(1);
    root.add_repeated();
    (*root.mutable_string_map())["key"].set_value(0);
    (*root.mutable_integer_map())[-42].set_value(0);
    (*root.mutable_bool_map())[true].set_value(0);
    root.mutable_type_rule();
    PopulateIgnoredCollections(*root.mutable_recursive());
    root.mutable_recursive()->mutable_singular();
    root.mutable_noise()->add_messages()->mutable_lt_rule()->set_seconds(100);
    ExpectSameValidation(root);
    const auto result = ValidateMessage(root);
    ASSERT_FALSE(result.IsSuccess());
    EXPECT_EQ(result.GetError().GetViolations().size(), 7);
}

UTEST(ValidationVisitor, AbsentFieldsAndOneof) {
    types::AbsentRules message;
    ExpectSameValidation(message);
    const auto result = ValidateMessage(message);
    ASSERT_FALSE(result.IsSuccess());
    EXPECT_EQ(result.GetError().GetViolations().size(), 5);
}

UTEST(ValidationVisitor, IgnoresOnlyTheSelectedEdges) {
    types::TraversalRoot root;
    root.mutable_ignored();
    root.add_ignored_items();
    root.add_ignored_items();
    (*root.mutable_ignored_values())["one"].set_value(0);
    (*root.mutable_ignored_values())["two"].set_value(0);
    root.add_ignored_repeated();
    (*root.mutable_ignored_map())["ignored"].set_value(0);
    ExpectSameValidation(root);
    EXPECT_TRUE(ValidateMessage(root).IsSuccess());

    // The same child type is still validated through another edge.
    root.mutable_singular();
    ExpectSameValidation(root);
    EXPECT_FALSE(ValidateMessage(root).IsSuccess());
}

UTEST(ValidationVisitor, DisabledLocalRulesStillVisitChildren) {
    types::DisabledRules message;
    ExpectSameValidation(message);
    EXPECT_TRUE(ValidateMessage(message).IsSuccess());
    message.mutable_nested();
    ExpectSameValidation(message);
    EXPECT_FALSE(ValidateMessage(message).IsSuccess());
}

UTEST(ValidationVisitor, CyclesAndCachedSubgraphs) {
    types::ValidationCycleA root;
    root.mutable_child()->mutable_back()->mutable_child()->mutable_checked();
    ExpectSameValidation(root);
    EXPECT_FALSE(ValidateMessage(root).IsSuccess());
    ExpectSameValidation(root.child());
    root.mutable_child()->mutable_back()->mutable_child()->mutable_checked()->set_value(1);
    ExpectSameValidation(root);
    EXPECT_TRUE(ValidateMessage(root).IsSuccess());
}

UTEST(ValidationVisitor, RulelessCycle) {
    types::RulelessCycle root;
    auto* node = &root;
    for (int i = 0; i < 20; ++i) {
        node->mutable_noise()->mutable_lt_rule()->set_seconds(100);
        node = node->mutable_next();
    }
    ExpectSameValidation(root);
    EXPECT_TRUE(ValidateMessage(root).IsSuccess());
}

UTEST(ValidationVisitor, SparsePopulatedMessage) {
    types::SparseRequest message;
    message.mutable_branch()->mutable_checked()->set_value(1);
    message.mutable_branch()->add_noise()->set_required_rule(0);
    message.add_siblings()->set_required_rule(0);
    (*message.mutable_lookup())["unconstrained"].set_required_rule(0);
    ExpectSameValidation(message);
    EXPECT_TRUE(ValidateMessage(message).IsSuccess());
    message.mutable_branch()->mutable_checked()->set_value(0);
    ExpectSameValidation(message);
    EXPECT_FALSE(ValidateMessage(message).IsSuccess());
}

UTEST(ValidationVisitor, CompilationErrorsOnlyOnPresentNonIgnoredChildren) {
    types::InvalidRuleParent message;
    message.mutable_ignored();
    ExpectSameValidation(message);
    EXPECT_TRUE(ValidateMessage(message).IsSuccess());
    message.mutable_child();
    ExpectSameValidation(message);
    ASSERT_FALSE(ValidateMessage(message).IsSuccess());
    EXPECT_EQ(ValidateMessage(message).GetError().GetType(), ValidationError::Type::kInternal);
}

UTEST(ValidationVisitor, AnyPayloadIsNotUnpacked) {
    google::protobuf::Any message;
    message.PackFrom(types::ValidationLeaf{});
    ExpectSameValidation(message);
    EXPECT_TRUE(ValidateMessage(message).IsSuccess());
}

UTEST(ValidationVisitor, ExtensionsDiscoveredAfterCompilation) {
    types::ExtensibleParent message;
    message.mutable_child();
    ExpectSameValidation(message);
    EXPECT_TRUE(ValidateMessage(message).IsSuccess());
    message.mutable_child()->MutableExtension(types::extension_leaf);
    ExpectSameValidation(message);
    EXPECT_FALSE(ValidateMessage(message).IsSuccess());
}

UTEST(ValidationVisitor, DenseRulesMatchOriginal) {
    ExpectSameValidation(tests::CreateValidMessage(2));
    ExpectSameValidation(tests::CreateInvalidMessage());
}

UTEST(ValidationVisitor, WarmupDefersErrorsAndAllowsUnseenTypes) {
    impl::ValidationVisitor visitor;
    visitor.Compile(*types::InvalidRuleParent::descriptor());
    visitor.Compile(*types::InvalidRuleParent::descriptor());

    types::InvalidRuleParent message;
    auto result = visitor.Validate(message, {});
    ASSERT_TRUE(result.ok());
    EXPECT_TRUE(result->success());

    message.mutable_child();
    EXPECT_FALSE(visitor.Validate(message, {}).ok());

    types::SparseRequest unseen;
    unseen.mutable_branch()->mutable_checked()->set_value(1);
    auto unseen_result = visitor.Validate(unseen, {});
    ASSERT_TRUE(unseen_result.ok());
    EXPECT_TRUE(unseen_result->success());
}

UTEST(ValidationVisitor, MessageCelReadsUnconstrainedFields) {
    types::CrossFieldRules message;
    message.set_limit(42);
    message.mutable_noise()->set_required_rule(42);
    ExpectSameValidation(message);
    EXPECT_TRUE(ValidateMessage(message).IsSuccess());
    message.mutable_noise()->set_required_rule(0);
    ExpectSameValidation(message);
    EXPECT_FALSE(ValidateMessage(message).IsSuccess());
}

UTEST_MT(ValidationVisitor, ConcurrentValidation, 4) {
    std::vector<engine::Task> tasks;
    tasks.reserve(32);
    for (int i = 0; i < 32; ++i) {
        tasks.push_back(utils::Async("validate-sparse", [] {
            types::SparseRequest message;
            for (int j = 0; j < 10; ++j) {
                message.mutable_branch()->mutable_checked()->set_value(j);
                EXPECT_EQ(ValidateMessage(message).IsSuccess(), j > 0);
            }
        }));
    }
    engine::WaitAllChecked(tasks);
}

}  // namespace grpc_protovalidate

USERVER_NAMESPACE_END
