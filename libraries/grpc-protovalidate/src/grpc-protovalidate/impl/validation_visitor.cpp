#include <grpc-protovalidate/impl/validation_visitor.hpp>

#include <algorithm>
#include <cstddef>
#include <utility>

#include <google/protobuf/descriptor.h>

#include <userver/utils/algo.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace grpc_protovalidate::impl {

// Implementation ideas:
// - ugrpc::VisitorCompiler (grpc/src/ugrpc/protobuf_visit.cpp): compile a descriptor graph and propagate
//   selection through reverse edges to identify subtrees that need validation.
// - buf::validate::Validator (protovalidate-cc, buf/validate/validator.cc): preserve singular/repeated/map
//   traversal, ignore rules, fail-fast behavior, and violation paths.
// Rule compilation and evaluation use protovalidate's existing internal API.

namespace {

// Match Validator::ValidateFields: ignore on items/values also suppresses recursion,
// while the rules on the containing field are still evaluated by NewMessageRules.
const google::protobuf::Descriptor* GetChildDescriptor(const google::protobuf::FieldDescriptor& field) {
    if (field.cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) {
        return nullptr;
    }
    const auto& rules = field.options().GetExtension(buf::validate::field);
    if (rules.ignore() == buf::validate::IGNORE_ALWAYS ||
        (rules.has_repeated() && rules.repeated().items().ignore() == buf::validate::IGNORE_ALWAYS) ||
        (rules.has_map() && rules.map().values().ignore() == buf::validate::IGNORE_ALWAYS))
    {
        return nullptr;
    }
    if (field.is_map()) {
        const auto* value = field.message_type()->map_value();
        return value->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE
                   ? value->message_type()
                   : nullptr;
    }
    return field.message_type();
}

}  // namespace

ValidationVisitor::ValidationVisitor() {
    auto builder = buf::validate::internal::NewRuleBuilder(&rules_arena_);
    UINVARIANT(builder.ok(), "Failed to create protovalidate rule builder");
    builder_ = std::move(builder).value();
}

void ValidationVisitor::Compile(const google::protobuf::Descriptor& descriptor) { GetOrCompile(descriptor); }

ValidationVisitor::FieldPlan ValidationVisitor::MakeFieldPlan(
    const google::protobuf::FieldDescriptor& field,
    MessagePlan& child
) {
    return FieldPlan{
        .field = &field,
        .child = &child,
        .map_key = field.is_map() ? field.message_type()->map_key() : nullptr,
        .map_value = field.is_map() ? field.message_type()->map_value() : nullptr,
    };
}

ValidationVisitor::MessagePlan& ValidationVisitor::GetOrCompile(const google::protobuf::Descriptor& descriptor) {
    if (const auto* plan_ptr = utils::FindOrNullptr(plans_, &descriptor)) {
        return **plan_ptr;
    }

    std::vector<MessagePlan*> pending;
    auto& root = AddPlan(descriptor, pending);
    const auto parents = CollectParents(pending);
    SelectParents(parents);
    for (auto* plan : pending) {
        PruneFields(*plan);
    }
    return root;
}

ValidationVisitor::MessagePlan& ValidationVisitor::AddPlan(
    const google::protobuf::Descriptor& descriptor,
    std::vector<MessagePlan*>& pending
) {
    if (const auto* plan_ptr = utils::FindOrNullptr(plans_, &descriptor)) {
        return **plan_ptr;
    }
    auto rules =
        buf::validate::internal::NewMessageRules(message_factory_, false, &rules_arena_, *builder_, &descriptor);
    // Extensions are discovered from the actual object, so their ancestors must remain reachable.
    const bool needs_validation = !rules.ok() || !rules->empty() || descriptor.extension_range_count() != 0;
    auto plan = std::make_unique<MessagePlan>(MessagePlan{
        .descriptor = &descriptor,
        .rules = std::move(rules),
        .fields = {},
        .needs_validation = needs_validation,
    });
    auto* plan_ptr = plan.get();
    plans_.emplace(&descriptor, std::move(plan));
    pending.push_back(plan_ptr);
    return *plan_ptr;
}

ValidationVisitor::Parents ValidationVisitor::CollectParents(std::vector<MessagePlan*>& pending) {
    // Build the entire reachable graph before propagating selection, including cycles.
    Parents parents;
    for (std::size_t i = 0; i < pending.size(); ++i) {
        auto& plan = *pending[i];
        for (int index = 0; index < plan.descriptor->field_count(); ++index) {
            const auto& field = *plan.descriptor->field(index);
            if (const auto* child_descriptor = GetChildDescriptor(field)) {
                auto& child = AddPlan(*child_descriptor, pending);
                plan.fields.push_back(MakeFieldPlan(field, child));
                parents[&child].push_back(&plan);
            }
        }
    }
    return parents;
}

void ValidationVisitor::SelectParents(const Parents& parents) {
    std::vector<MessagePlan*> selected;
    for (const auto& [child, parent_list] : parents) {
        if (child->needs_validation) {
            selected.push_back(child);
        }
    }
    for (std::size_t i = 0; i < selected.size(); ++i) {
        if (const auto* parent_list_ptr = utils::FindOrNullptr(parents, selected[i])) {
            for (auto* parent : *parent_list_ptr) {
                if (!parent->needs_validation) {
                    parent->needs_validation = true;
                    selected.push_back(parent);
                }
            }
        }
    }
}

void ValidationVisitor::PruneFields(MessagePlan& plan) {
    std::erase_if(plan.fields, [](const FieldPlan& field) { return !field.child->needs_validation; });
    // Reflection::ListFields orders fields by number, including for fail_fast.
    std::ranges::sort(plan.fields, {}, [](const FieldPlan& field) { return field.field->number(); });
}

ProtoValidationResult ValidationVisitor::Validate(
    const google::protobuf::Message& message,
    const ValidationOptions& options
) {
    const auto& plan = GetOrCompile(*message.GetDescriptor());
    if (!plan.needs_validation) {
        return buf::validate::ValidationResult{{}};
    }
    google::protobuf::Arena arena;
    buf::validate::internal::RuleContext ctx;
    ctx.arena = &arena;
    ctx.failFast = options.fail_fast;
    auto status = Visit(ctx, message, plan);
    if (!status.ok()) {
        return status;
    }
    ctx.finalize();
    return buf::validate::ValidationResult{std::move(ctx.violations)};
}

ValidationStatus ValidationVisitor::Visit(
    buf::validate::internal::RuleContext& ctx,
    const google::protobuf::Message& message,
    const MessagePlan& plan
) {
    if (!plan.rules.ok()) {
        return plan.rules.status();
    }
    for (const auto& rule : plan.rules.value()) {
        auto status = rule->Validate(ctx, message);
        if (ctx.shouldReturn(status)) {
            return status;
        }
    }
    if (plan.descriptor->extension_range_count() != 0) {
        return VisitExtensible(ctx, message);
    }
    for (const auto& field : plan.fields) {
        auto status = VisitField(ctx, message, field);
        if (ctx.shouldReturn(status)) {
            return status;
        }
    }
    return {};
}

ValidationStatus ValidationVisitor::VisitField(
    buf::validate::internal::RuleContext& ctx,
    const google::protobuf::Message& message,
    const FieldPlan& plan
) {
    const auto* reflection = message.GetReflection();
    if (!plan.field->is_repeated() && !reflection->HasField(message, plan.field)) {
        return {};
    }
    const int size = plan.field->is_repeated() ? reflection->FieldSize(message, plan.field) : 1;
    for (int i = 0; i < size; ++i) {
        const auto& item =
            plan.field->is_repeated()
                ? reflection->GetRepeatedMessage(message, plan.field, i)
                : reflection->GetMessage(message, plan.field);
        const auto& child = plan.map_value ? item.GetReflection()->GetMessage(item, plan.map_value) : item;
        const auto start = ctx.violations.size();
        auto status = Visit(ctx, child, *plan.child);
        if (ctx.violations.size() > start) {
            auto element = buf::validate::internal::fieldPathElement(plan.field);
            if (plan.map_value) {
                auto path_status =
                    buf::validate::internal::setPathElementMapKey(&element, item, plan.map_key, plan.map_value);
                if (!path_status.ok()) {
                    return path_status;
                }
            } else if (plan.field->is_repeated()) {
                element.set_index(i);
            }
            ctx.appendFieldPathElement(element, start);
        }
        if (ctx.shouldReturn(status)) {
            return status;
        }
    }
    return {};
}

ValidationStatus ValidationVisitor::VisitExtensible(
    buf::validate::internal::RuleContext& ctx,
    const google::protobuf::Message& message
) {
    // Keep protobuf extensions working, even when their descriptors were not reachable at compilation time.
    std::vector<const google::protobuf::FieldDescriptor*> fields;
    message.GetReflection()->ListFields(message, &fields);
    for (const auto* field : fields) {
        if (const auto* descriptor = GetChildDescriptor(*field)) {
            auto& child = GetOrCompile(*descriptor);
            if (child.needs_validation) {
                auto status = VisitField(ctx, message, MakeFieldPlan(*field, child));
                if (ctx.shouldReturn(status)) {
                    return status;
                }
            }
        }
    }
    return {};
}

}  // namespace grpc_protovalidate::impl

USERVER_NAMESPACE_END
