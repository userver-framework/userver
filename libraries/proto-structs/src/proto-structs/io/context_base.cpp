#include <userver/proto-structs/io/context.hpp>

#include <fmt/format.h>
#include <google/protobuf/descriptor.h>

#include <userver/utils/impl/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

std::string Context::GetCurrentPath(int current_field_number) const {
    const ::google::protobuf::Descriptor* last_message_desc = nullptr;
    auto result = GetCurrentPathImpl(last_message_desc);

    if (last_message_desc) {
        auto field_desc = last_message_desc->FindFieldByNumber(current_field_number);

        if (field_desc) {
            result.append(fmt::format(".{}", field_desc->name()));
        } else {
            result.append(fmt::format(".<unknown_{}>", current_field_number));
        }
    }

    return result;
}

std::string Context::GetCurrentPath(const ::google::protobuf::FieldDescriptor& current_field_desc) const {
    const ::google::protobuf::Descriptor* last_message_desc = nullptr;
    auto result = GetCurrentPathImpl(last_message_desc);

    if (last_message_desc) {
        if (current_field_desc.containing_type() == last_message_desc) {
            result.append(fmt::format(".{}", current_field_desc.name()));
        } else {
            result.append(fmt::format(".<unknown_{}>", current_field_desc.number()));
        }
    }

    return result;
}

std::string Context::GetCurrentPath() const {
    const ::google::protobuf::Descriptor* last_message_desc = nullptr;
    return GetCurrentPathImpl(last_message_desc);
}

std::string Context::GetCurrentPathImpl(const ::google::protobuf::Descriptor*& last_message_desc) const {
    auto current_ptr = &top_message_desc_;

    std::string result;
    result.reserve(128);
    result = current_ptr->full_name();

    for (const auto& field_number : path_) {
        auto field_desc = current_ptr->FindFieldByNumber(field_number);

        if (field_desc && field_desc->type() == ::google::protobuf::FieldDescriptor::TYPE_MESSAGE) {
            result.append(fmt::format(".{}", field_desc->name()));
            current_ptr = field_desc->message_type();
        } else {
            result.append(fmt::format(".<unknown_{}>", field_number));
            current_ptr = nullptr;
            break;
        }
    }

    last_message_desc = current_ptr;
    return result;
}

template <typename TError>
const std::vector<TError>& ContextWithErrors<TError>::GetErrors(utils::impl::InternalTag) const& noexcept {
    return errors_;
}

template <typename TError>
std::vector<TError>&& ContextWithErrors<TError>::GetErrors(utils::impl::InternalTag) && noexcept {
    return std::move(errors_);
}

template <typename TError>
void ContextWithErrors<TError>::AddError(int field_number, std::string_view reason) {
    AddError(GetCurrentPath(field_number), reason);
}

template <typename TError>
void ContextWithErrors<
    TError>::AddError(const ::google::protobuf::FieldDescriptor& field_desc, std::string_view reason) {
    AddError(GetCurrentPath(field_desc), reason);
}

template <typename TError>
void ContextWithErrors<TError>::AddError(std::string_view reason) {
    AddError(GetCurrentPath(), reason);
}

template <typename TError>
void ContextWithErrors<TError>::AddError(std::string_view path, std::string_view reason) {
    errors_.emplace_back(path, reason);
    throw Error{errors_.back()};
}

template class ContextWithErrors<ReadError>;
template class ContextWithErrors<WriteError>;

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
