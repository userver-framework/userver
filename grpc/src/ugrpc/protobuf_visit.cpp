#include <userver/ugrpc/protobuf_visit.hpp>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/port.h>
#include <google/protobuf/reflection.h>

#include <ugrpc/impl/protobuf_utils.hpp>
#include <userver/ugrpc/impl/protobuf_collector.hpp>
#include <userver/ugrpc/protobuf_visit.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

namespace {

constexpr int kMaxRecursionLimit = 100;

void VisitMessagesRecursiveImpl(
    google::protobuf::Message& message,
    MessageVisitCallback callback,
    const int recursion_limit
) {
    UINVARIANT(recursion_limit > 0, "Recursion limit reached while traversing protobuf Message.");

    // Apply to this message
    callback(message);

    // Loop over nested structs
    VisitFields(
        message,
        [&callback,
         &recursion_limit](google::protobuf::Message& message, const google::protobuf::FieldDescriptor& field) -> void {
            // Not a nested message
            if (!impl::IsMessage(field)) return;

            VisitNestedMessage(message, field, [&](google::protobuf::Message& msg) {
                VisitMessagesRecursiveImpl(msg, callback, recursion_limit - 1);
            });
        }
    );
}

void GetNestedMessageDescriptorsImpl(
    const google::protobuf::Descriptor& descriptor,
    std::unordered_set<const google::protobuf::Descriptor*>& result
) {
    if (!result.insert(&descriptor).second) {
        // It is already there. Stop to avoid infinite recursion.
        return;
    }

    // Check the fields
    for (const google::protobuf::FieldDescriptor* field : GetFieldDescriptors(descriptor)) {
        UINVARIANT(field, "field is nullptr");

        // Not a nested message
        if (!impl::IsMessage(*field)) continue;

        const google::protobuf::Descriptor* msg = field->message_type();
        UINVARIANT(msg, "msg is nullptr");
        GetNestedMessageDescriptorsImpl(*msg, result);
    }
}

std::unordered_set<const google::protobuf::Descriptor*> GetNestedMessageDescriptorsSet(
    const google::protobuf::Descriptor& descriptor
) {
    std::unordered_set<const google::protobuf::Descriptor*> result;
    GetNestedMessageDescriptorsImpl(descriptor, result);
    return result;
}

}  // namespace

void VisitFields(google::protobuf::Message& message, FieldVisitCallback callback) {
    // Get reflection
    const google::protobuf::Reflection* reflection = message.GetReflection();
    UINVARIANT(reflection, "reflection is nullptr");

    std::vector<const google::protobuf::FieldDescriptor*> fields;
    reflection->ListFields(message, &fields);

    for (const google::protobuf::FieldDescriptor* field : fields) {
        UINVARIANT(field, "field is nullptr");
        callback(message, *field);
    }
}

void VisitMessagesRecursive(google::protobuf::Message& message, MessageVisitCallback callback) {
    VisitMessagesRecursiveImpl(message, callback, kMaxRecursionLimit);
}

void VisitFieldsRecursive(google::protobuf::Message& message, FieldVisitCallback callback) {
    VisitMessagesRecursiveImpl(
        message, [&](google::protobuf::Message& message) -> void { VisitFields(message, callback); }, kMaxRecursionLimit
    );
}

void VisitNestedMessage(
    google::protobuf::Message& message,
    const google::protobuf::FieldDescriptor& field,
    MessageVisitCallback callback
) {
    UINVARIANT(impl::IsMessage(field), "Not a nested message");

    // Get reflection
    const google::protobuf::Reflection* reflection = message.GetReflection();
    UINVARIANT(reflection, "reflection is nullptr");

    if (field.is_repeated()) {
        // Repeated types (including maps)
        const int repeated_size = reflection->FieldSize(message, &field);
        for (int i = 0; i < repeated_size; ++i) {
            google::protobuf::Message* msg = reflection->MutableRepeatedMessage(&message, &field, i);
            UINVARIANT(msg, "msg is nullptr");
            callback(*msg);
        }
    } else if (reflection->HasField(message, &field)) {
        // Primitive types
        google::protobuf::Message* msg = reflection->MutableMessage(&message, &field);
        UINVARIANT(msg, "msg is nullptr");
        callback(*msg);
    }
}

FieldDescriptorList GetFieldDescriptors(const google::protobuf::Descriptor& descriptor) {
    FieldDescriptorList result;
    result.reserve(descriptor.field_count());
    for (int idx = 0; idx < descriptor.field_count(); ++idx) {
        const google::protobuf::FieldDescriptor* field = descriptor.field(idx);
        UINVARIANT(field, "field is nullptr");
        result.push_back(field);
    }
    return result;
}

DescriptorList GetNestedMessageDescriptors(const google::protobuf::Descriptor& descriptor) {
    const auto set = GetNestedMessageDescriptorsSet(descriptor);
    return DescriptorList(set.begin(), set.end());
}

const google::protobuf::Descriptor* FindGeneratedMessage(std::string_view name) {
    const google::protobuf::DescriptorPool* pool = google::protobuf::DescriptorPool::generated_pool();
    UINVARIANT(pool, "pool is nullptr");
#if GOOGLE_PROTOBUF_VERSION >= 3022000
    return pool->FindMessageTypeByName(name);
#else
    return pool->FindMessageTypeByName(std::string(name));
#endif
}

const google::protobuf::FieldDescriptor*
FindField(const google::protobuf::Descriptor* descriptor, std::string_view field) {
    UINVARIANT(descriptor, "descriptor is nullptr");
#if GOOGLE_PROTOBUF_VERSION >= 3022000
    return descriptor->FindFieldByName(field);
#else
    return descriptor->FindFieldByName(std::string(field));
#endif
}

void VisitorCompiler::Compile(const google::protobuf::Descriptor* descriptor) {
    UINVARIANT(descriptor, "descriptor is nullptr");
    Compile(DescriptorList{descriptor});
}

void VisitorCompiler::Compile(const DescriptorList& descriptors) {
    {
        bool are_compiled = true;
        std::shared_lock read_lock = LockRead();
        for (const google::protobuf::Descriptor* descriptor : descriptors) {
            if (compiled_.find(descriptor) == compiled_.end()) {
                // Something is not compiled. Need to compile.
                are_compiled = false;
                break;
            }
        }
        if (are_compiled) {
            // Everything is already compiled. Stop.
            return;
        }
    }

    std::unique_lock write_lock = LockWrite();
    for (const google::protobuf::Descriptor* descriptor : GetFullSubtrees(descriptors)) {
        UINVARIANT(descriptor, "descriptor is nullptr");

        // We have already compiled this. Skip.
        if (!compiled_.insert(descriptor).second) continue;

        // Compile the selection data
        CompileOne(*descriptor);

        // Update everything else
        for (const google::protobuf::FieldDescriptor* field : GetFieldDescriptors(*descriptor)) {
            UINVARIANT(field, "field is nullptr");

            // Not a nested message
            if (!impl::IsMessage(*field)) continue;

            // Sync the reverse edges.
            // Even from unknown types - we might need to compile them in the future.
            const google::protobuf::Descriptor* msg = field->message_type();
            UINVARIANT(msg, "msg is nullptr");
            reverse_edges_[msg].insert(field);

            // Compile the direct edge
            propagated_.erase(msg);
            PropagateSelected(msg);
        }

        // Compile the connections to this message using the reverse edges
        PropagateSelected(descriptor);
    }
}

void VisitorCompiler::CompileAllGenerated() { Compile(impl::GetGeneratedMessages()); }

void VisitorCompiler::CompileGenerated(std::string_view message_name) { Compile(FindGeneratedMessage(message_name)); }

void VisitorCompiler::CompileGenerated(utils::span<std::string_view> message_names) {
    DescriptorList descriptors;
    for (const std::string_view& message_name : message_names) {
        descriptors.push_back(FindGeneratedMessage(message_name));
    }
    Compile(descriptors);
}

bool VisitorCompiler::ContainsSelected(const google::protobuf::Descriptor* descriptor) {
    UINVARIANT(descriptor, "descriptor is nullptr");

    // Compile if not yet compiled
    Compile(descriptor);

    std::shared_lock read_lock = LockRead();
    return fields_with_selected_children_.find(descriptor) != fields_with_selected_children_.end() ||
           IsSelected(*descriptor);
}

std::shared_lock<std::shared_mutex> VisitorCompiler::LockRead() {
    std::shared_lock read_lock(mutex_, std::defer_lock);
    if (lock_behavior_ == LockBehavior::kShared) read_lock.lock();
    return read_lock;
}

std::unique_lock<std::shared_mutex> VisitorCompiler::LockWrite() {
    std::unique_lock write_lock(mutex_, std::defer_lock);
    if (lock_behavior_ == LockBehavior::kShared) write_lock.lock();
    return write_lock;
}

VisitorCompiler::DescriptorSet VisitorCompiler::GetFullSubtrees(const DescriptorList& descriptors) const {
    DescriptorSet result;
    for (const google::protobuf::Descriptor* descriptor : descriptors) {
        UINVARIANT(descriptor, "descriptor is nullptr");
        if (result.find(descriptor) != result.end()) {
            // We have already parsed this
            continue;
        }
        result.merge(GetNestedMessageDescriptorsSet(*descriptor));
    }
    return result;
}

void VisitorCompiler::PropagateSelected(const google::protobuf::Descriptor* descriptor) {
    UINVARIANT(descriptor, "descriptor is nullptr");
    if (!IsSelected(*descriptor) &&
        fields_with_selected_children_.find(descriptor) == fields_with_selected_children_.end()) {
        // This does not need to be propagated
        return;
    }

    if (!propagated_.insert(descriptor).second) {
        // We have already propagated this before
        return;
    }

    const auto it = reverse_edges_.find(descriptor);
    if (it == reverse_edges_.end()) return;  // No edges

    const FieldDescriptorSet& fields = it->second;
    for (const google::protobuf::FieldDescriptor* field : fields) {
        UINVARIANT(field, "field is nullptr");

        const google::protobuf::Descriptor* msg = field->containing_type();
        UINVARIANT(msg, "msg is nullptr");

        // Save the connection
        fields_with_selected_children_[msg].insert(field);

        // Go further over reverse_edges
        PropagateSelected(msg);
    }
}

FieldsVisitor::FieldsVisitor(Selector selector)
    : BaseVisitor<FieldVisitCallback>(LockBehavior::kShared), selector_(selector) {
    CompileAllGenerated();
}

FieldsVisitor::FieldsVisitor(Selector selector, const DescriptorList& descriptors)
    : BaseVisitor<FieldVisitCallback>(LockBehavior::kShared), selector_(selector) {
    Compile(descriptors);
}

FieldsVisitor::FieldsVisitor(Selector selector, LockBehavior lock_behavior)
    : BaseVisitor<FieldVisitCallback>(lock_behavior), selector_(selector) {
    CompileAllGenerated();
}

FieldsVisitor::FieldsVisitor(Selector selector, const DescriptorList& descriptors, LockBehavior lock_behavior)
    : BaseVisitor<FieldVisitCallback>(lock_behavior), selector_(selector) {
    Compile(descriptors);
}

void FieldsVisitor::CompileOne(const google::protobuf::Descriptor& descriptor) {
    for (const google::protobuf::FieldDescriptor* field : GetFieldDescriptors(descriptor)) {
        UINVARIANT(field, "field is nullptr");
        if (selector_(*field)) selected_fields_[&descriptor].insert(field);
    }
}

void FieldsVisitor::DoVisit(google::protobuf::Message& message, FieldVisitCallback callback) const {
    const auto it = selected_fields_.find(message.GetDescriptor());
    if (it == selected_fields_.end()) return;

    // Get reflection
    const google::protobuf::Reflection* reflection = message.GetReflection();
    UINVARIANT(reflection, "reflection is nullptr");

    const FieldDescriptorSet& fields = it->second;
    for (const google::protobuf::FieldDescriptor* field : fields) {
        // Repeated types (including maps)
        if (field->is_repeated()) {
            if (reflection->FieldSize(message, field) > 0) {
                callback(message, *field);
            }
        } else {
            // Primitive types
            if (reflection->HasField(message, field)) {
                callback(message, *field);
            }
        }
    }
}

MessagesVisitor::MessagesVisitor(Selector selector)
    : BaseVisitor<MessageVisitCallback>(LockBehavior::kShared), selector_(selector) {
    CompileAllGenerated();
}

MessagesVisitor::MessagesVisitor(Selector selector, const DescriptorList& descriptors)
    : BaseVisitor<MessageVisitCallback>(LockBehavior::kShared), selector_(selector) {
    Compile(descriptors);
}

MessagesVisitor::MessagesVisitor(Selector selector, LockBehavior lock_behavior)
    : BaseVisitor<MessageVisitCallback>(lock_behavior), selector_(selector) {
    CompileAllGenerated();
}

MessagesVisitor::MessagesVisitor(Selector selector, const DescriptorList& descriptors, LockBehavior lock_behavior)
    : BaseVisitor<MessageVisitCallback>(lock_behavior), selector_(selector) {
    Compile(descriptors);
}

void MessagesVisitor::CompileOne(const google::protobuf::Descriptor& descriptor) {
    if (selector_(descriptor)) {
        selected_messages_.insert(&descriptor);
    }
}

void MessagesVisitor::DoVisit(google::protobuf::Message& message, MessageVisitCallback callback) const {
    const auto it = selected_messages_.find(message.GetDescriptor());
    if (it == selected_messages_.end()) return;
    callback(message);
}

}  // namespace ugrpc

USERVER_NAMESPACE_END
