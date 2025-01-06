#include <userver/ugrpc/field_mask.hpp>

#include <algorithm>
#include <cstddef>
#include <utility>

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/field_mask.pb.h>
#include <google/protobuf/message.h>
#include <google/protobuf/port.h>
#include <google/protobuf/reflection.h>

#include <ugrpc/impl/protobuf_utils.hpp>
#include <userver/ugrpc/protobuf_visit.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/text_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

namespace {

struct Part {
    std::string_view part;
    std::size_t used_symbols;
};

Part GetRoot(std::string_view path) {
    if (path.empty()) {
        throw FieldMask::BadPathError("The path may not be empty");
    }

    if (path[0] == '`') {
        const std::size_t end = path.find('`', 1);
        if (end == std::string_view::npos) {
            throw FieldMask::BadPathError("Every backtick must be closed");
        }
        if (end != path.size() - 1 && path[end + 1] != '.') {
            throw FieldMask::BadPathError("A closing backtick must be followed by a dot");
        }

        const std::string_view part = path.substr(1, end - 1);
        if (part.empty()) {
            throw FieldMask::BadPathError("The path may not have empty parts");
        }

        return {part, std::min(end + 2, path.size())};
    }

    const std::size_t end = std::min(path.find('.'), path.size());
    const std::string_view part = path.substr(0, end);
    if (part.empty()) {
        throw FieldMask::BadPathError("The path may not have empty parts");
    }
    return {part, std::min(end + 1, path.size())};
}

std::string GetMapKeyAsString(const google::protobuf::Message& entry) {
    const google::protobuf::Descriptor* descriptor = entry.GetDescriptor();
    UINVARIANT(descriptor, "descriptor is nullptr");

    const google::protobuf::FieldDescriptor* map_key = descriptor->map_key();
    UINVARIANT(map_key, "map_key is nullptr");

    const google::protobuf::Reflection* reflection = entry.GetReflection();
    UINVARIANT(reflection, "reflection is nullptr");

    switch (map_key->cpp_type()) {
        case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
            return std::to_string(reflection->GetInt32(entry, map_key));
        case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
            return std::to_string(reflection->GetInt64(entry, map_key));
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
            return std::to_string(reflection->GetUInt32(entry, map_key));
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
            return std::to_string(reflection->GetUInt64(entry, map_key));
        case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
            return reflection->GetString(entry, map_key);
        default:
            UINVARIANT(false, "Invalid map_key type");
    }
}

}  // namespace

FieldMask::FieldMask(const google::protobuf::FieldMask& field_mask) {
    for (const auto& path : field_mask.paths()) {
        AddPath(path);
    }
}

void FieldMask::AddPath(std::string_view path) {
    if (path.empty() || is_leaf_) {
        // Everything inside is already in the field mask
        children_->clear();
        is_leaf_ = true;
        return;
    }

    Part root = GetRoot(path);
    const auto it = children_->emplace(std::move(root.part), FieldMask());
    it.first->second.AddPath(path.substr(root.used_symbols));
}

google::protobuf::FieldMask FieldMask::ToGoogleMask() const {
    google::protobuf::FieldMask out;
    std::vector<std::string> stack;
    ToGoogleMaskImpl(stack, out);
    return out;
}

void FieldMask::ToGoogleMaskImpl(std::vector<std::string>& stack, google::protobuf::FieldMask& out) const {
    if (IsLeaf()) {
        const std::string path = fmt::format("{}", fmt::join(stack, "."));
        return out.add_paths(path);
    }
    for (const auto& [field_name, nested_mask] : *children_) {
        if (field_name.find('.') == std::string::npos) {
            stack.push_back(field_name);
        } else {
            stack.push_back(fmt::format("`{}`", field_name));
        }
        nested_mask.ToGoogleMaskImpl(stack, out);
        stack.pop_back();
    }
}

void FieldMask::CheckValidity(const google::protobuf::Descriptor* descriptor) const {
    // A leaf mask is surely valid
    if (IsLeaf()) return;

    // It is not possible to mask nothing
    if (!descriptor) {
        throw BadPathError("Trying to mask a NULL descriptor");
    }

    for (const auto& [field_name, nested_mask] : *children_) {
        const google::protobuf::FieldDescriptor* field = descriptor->FindFieldByName(field_name);

        // Check that the field exists
        if (!field) {
            throw BadPathError(fmt::format("{} was not found on message {}", field_name, descriptor->full_name()));
        }

        // If the nested mask is a leaf, it is surely valid
        if (nested_mask.IsLeaf()) continue;

        if (field->is_map()) {
            // The children of nested_mask are map keys

            const google::protobuf::Descriptor* entry = field->message_type();
            UINVARIANT(entry, "entry is nullptr");

            const google::protobuf::FieldDescriptor* map_key = entry->map_key();
            UINVARIANT(map_key, "map_key is nullptr");

            using Fd = google::protobuf::FieldDescriptor;
            if (map_key->cpp_type() != Fd::CPPTYPE_INT32 && map_key->cpp_type() != Fd::CPPTYPE_INT64 &&
                map_key->cpp_type() != Fd::CPPTYPE_UINT32 && map_key->cpp_type() != Fd::CPPTYPE_UINT64 &&
                map_key->cpp_type() != Fd::CPPTYPE_STRING) {
                throw BadPathError(fmt::format(
                    "Trying to mask a map with non-integer and non-strings "
                    "keys at field {} of message {}",
                    field->name(),
                    descriptor->full_name()
                ));
            }

            const google::protobuf::FieldDescriptor* map_value = entry->map_value();
            UINVARIANT(map_value, "map_value is nullptr");

            for (const auto& [key, value_mask] : *nested_mask.children_) {
                // If the mask is a leaf, it is surely valid
                if (value_mask.IsLeaf()) continue;

                // It is not possible to mask a non-message type.
                if (!impl::IsMessage(*map_value)) {
                    throw BadPathError(fmt::format(
                        "Trying to mask a primitive map value at key {} in "
                        "field {} of message {}",
                        key,
                        field->name(),
                        descriptor->full_name()
                    ));
                }

                value_mask.CheckValidity(map_value->message_type());
            }
        } else if (field->is_repeated()) {
            if (nested_mask.children_->size() != 1 || nested_mask.children_->begin()->first != "*") {
                throw BadPathError(fmt::format(
                    "A non-leaf mask for a repeated field {} in message {} "
                    "contains something except *",
                    field->name(),
                    descriptor->full_name()
                ));
            }

            const FieldMask& star_mask = nested_mask.children_->begin()->second;

            // If the mask is a leaf, it is surely valid
            if (star_mask.IsLeaf()) continue;

            if (!impl::IsMessage(*field)) {
                throw BadPathError(fmt::format(
                    "Trying to mask a repeated of primitives in field {} of message {}",
                    field->name(),
                    descriptor->full_name()
                ));
            }

            star_mask.CheckValidity(field->message_type());
        } else if (!impl::IsMessage(*field)) {
            // It is not possible to mask a non-message type.
            throw BadPathError(
                fmt::format("Trying to mask a primitive field {} of message {}", field->name(), descriptor->full_name())
            );
        } else {
            nested_mask.CheckValidity(field->message_type());
        }
    }
}

bool FieldMask::IsPathFullyIn(std::string_view path) const {
    if (path.empty() || IsLeaf()) return IsLeaf();
    const Part root = GetRoot(path);
    const auto it = utils::impl::FindTransparent(*children_, root.part);
    if (it == children_->end()) return false;
    return it->second.IsPathFullyIn(path.substr(root.used_symbols));
}

bool FieldMask::IsPathPartiallyIn(std::string_view path) const {
    if (path.empty() || IsLeaf()) return true;
    const Part root = GetRoot(path);
    const auto it = utils::impl::FindTransparent(*children_, root.part);
    if (it == children_->end()) return false;
    return it->second.IsPathPartiallyIn(path.substr(root.used_symbols));
}

void FieldMask::Trim(google::protobuf::Message& message) const {
    CheckValidity(message.GetDescriptor());
    TrimNoValidate(message);
}

void FieldMask::TrimNoValidate(google::protobuf::Message& message) const {
    if (IsLeaf()) {
        // The message is either explicitly in the mask, or the mask is empty
        return;
    }

    const google::protobuf::Reflection* reflection = message.GetReflection();
    UINVARIANT(reflection, "reflection is nullptr");

    VisitFields(message, [&](google::protobuf::Message&, const google::protobuf::FieldDescriptor& field) {
        const auto it = utils::impl::FindTransparent(*children_, field.name());
        if (it == children_->end()) {
            // The field is not in the field mask. Remove it.
            return reflection->ClearField(&message, &field);
        }

        const FieldMask& nested_mask = it->second;
        if (nested_mask.IsLeaf()) {
            // The field must not be masked
            return;
        }

        if (field.is_map()) {
            const google::protobuf::MutableRepeatedFieldRef<google::protobuf::Message> map =
                reflection->GetMutableRepeatedFieldRef<google::protobuf::Message>(&message, &field);

            const auto star_mask_it = nested_mask.children_->find("*");
            for (int i = 0; i < map.size(); ++i) {
                google::protobuf::Message* entry = reflection->MutableRepeatedMessage(&message, &field, i);
                UINVARIANT(entry, "entry is nullptr");

                const std::string key = GetMapKeyAsString(*entry);
                const auto value_mask_it = utils::impl::FindTransparent(*nested_mask.children_, key);

                const auto& actual_mask_it =
                    value_mask_it != nested_mask.children_->end() ? value_mask_it : star_mask_it;
                if (actual_mask_it == nested_mask.children_->end()) {
                    // The map key is not in the field mask.
                    // Remove the record by putting it to the back of the array.
                    //
                    // The protocol does not guarantee the order of elements in the
                    // map, so this should be safe.
                    map.SwapElements(i--, map.size() - 1);
                    map.RemoveLast();
                    continue;
                }

                const FieldMask& actual_mask = actual_mask_it->second;
                if (actual_mask.IsLeaf()) continue;

                // The map key has a mask for the value
                const google::protobuf::Descriptor* entry_desc = field.message_type();
                UINVARIANT(entry_desc, "entry_desc is nullptr");

                const google::protobuf::FieldDescriptor* map_value = entry_desc->map_value();
                UINVARIANT(map_value, "map_value is nullptr");
                UINVARIANT(impl::IsMessage(*map_value), "non-leaf mask may only apply to messages");

                const google::protobuf::Reflection* entry_reflection = entry->GetReflection();
                UINVARIANT(entry_reflection, "entry_reflection is nullptr");

                google::protobuf::Message* value_msg = entry_reflection->MutableMessage(entry, map_value);
                UINVARIANT(value_msg, "value_msg is nullptr");
                actual_mask.TrimNoValidate(*value_msg);
            }
        } else if (field.is_repeated()) {
            constexpr std::string_view kBadRepeatedMask = "A non-leaf field mask for an array can contain only *";
            UINVARIANT(nested_mask.children_->size() == 1, kBadRepeatedMask);
            UINVARIANT(nested_mask.children_->begin()->first == "*", kBadRepeatedMask);

            const FieldMask& star_mask = nested_mask.children_->begin()->second;
            if (star_mask.IsLeaf()) return;

            UINVARIANT(impl::IsMessage(field), "non-leaf mask may only apply to messages");

            const int repeated_size = reflection->FieldSize(message, &field);
            for (int i = 0; i < repeated_size; ++i) {
                google::protobuf::Message* repeated_msg = reflection->MutableRepeatedMessage(&message, &field, i);
                UINVARIANT(repeated_msg, "repeated_msg is nullptr");
                star_mask.TrimNoValidate(*repeated_msg);
            }
        } else {
            UINVARIANT(impl::IsMessage(field), "non-leaf mask may only apply to messages");
            google::protobuf::Message* nested_msg = reflection->MutableMessage(&message, &field);
            UINVARIANT(nested_msg, "nested_msg is nullptr");
            nested_mask.TrimNoValidate(*nested_msg);
        }
    });
}

bool FieldMask::IsLeaf() const { return is_leaf_ || children_->empty(); }

std::vector<std::string_view> FieldMask::GetFieldNamesList() const {
    const auto view = GetFieldNames();
    return std::vector<std::string_view>(view.cbegin(), view.cend());
}

bool FieldMask::HasFieldName(std::string_view field) const {
    return utils::impl::FindTransparent(*children_, field) != children_->end();
}

utils::OptionalRef<const FieldMask> FieldMask::GetMaskForField(std::string_view field) const {
    const auto it = utils::impl::FindTransparent(*children_, field);
    if (it == children_->end()) return std::nullopt;
    return it->second;
}

}  // namespace ugrpc

USERVER_NAMESPACE_END
