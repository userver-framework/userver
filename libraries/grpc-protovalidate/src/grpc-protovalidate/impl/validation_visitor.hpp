#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include <userver/grpc-protovalidate/buf_validate.hpp>

USERVER_NAMESPACE_BEGIN

namespace grpc_protovalidate::impl {

#ifdef ARCADIA_ROOT
using ValidationStatus = y_absl::Status;
using ProtoValidationResult = y_absl::StatusOr<buf::validate::ValidationResult>;
#else
using ValidationStatus = absl::Status;
using ProtoValidationResult = absl::StatusOr<buf::validate::ValidationResult>;
#endif

struct ValidationOptions {
    bool fail_fast{false};
};

// Compiles local protovalidate rules and the edges leading to rules. Not thread-safe:
// keep the visitor in compiler::ThreadLocal and hold its scope throughout validation.
class ValidationVisitor final {
public:
    ValidationVisitor();

    // Precompile this message type and its reachable children. Errors are cached and
    // reported by Validate only when the corresponding message is actually visited.
    void Compile(const google::protobuf::Descriptor& descriptor);

    ProtoValidationResult Validate(const google::protobuf::Message& message, const ValidationOptions& options);

private:
    struct MessagePlan;

    struct FieldPlan {
        const google::protobuf::FieldDescriptor* field;
        MessagePlan* child;
        const google::protobuf::FieldDescriptor* map_key;
        const google::protobuf::FieldDescriptor* map_value;
    };

    struct MessagePlan {
        const google::protobuf::Descriptor* descriptor;
        buf::validate::internal::Rules rules;
        std::vector<FieldPlan> fields;
        bool needs_validation;
    };

    using Parents = std::unordered_map<MessagePlan*, std::vector<MessagePlan*>>;

    MessagePlan& GetOrCompile(const google::protobuf::Descriptor& descriptor);

    MessagePlan& AddPlan(const google::protobuf::Descriptor& descriptor, std::vector<MessagePlan*>& pending);

    Parents CollectParents(std::vector<MessagePlan*>& pending);

    static void SelectParents(const Parents& parents);

    static void PruneFields(MessagePlan& plan);

    static FieldPlan MakeFieldPlan(const google::protobuf::FieldDescriptor& field, MessagePlan& child);

    ValidationStatus Visit(
        buf::validate::internal::RuleContext& ctx,
        const google::protobuf::Message& message,
        const MessagePlan& plan
    );

    ValidationStatus VisitField(
        buf::validate::internal::RuleContext& ctx,
        const google::protobuf::Message& message,
        const FieldPlan& plan
    );

    ValidationStatus VisitExtensible(
        buf::validate::internal::RuleContext& ctx,
        const google::protobuf::Message& message
    );

    // The compiled rules and CEL builder borrow allocations from this arena.
    google::protobuf::Arena rules_arena_;
    std::unique_ptr<buf::validate::internal::MessageFactory> message_factory_;
    std::unique_ptr<google::api::expr::runtime::CelExpressionBuilder> builder_;
    std::unordered_map<const google::protobuf::Descriptor*, std::unique_ptr<MessagePlan>> plans_;
};

}  // namespace grpc_protovalidate::impl

USERVER_NAMESPACE_END
