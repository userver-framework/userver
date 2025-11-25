#pragma once

/// @file userver/proto-structs/io/context.hpp
/// @brief Context base classes providing methods for conversion process management.

#include <string>
#include <string_view>
#include <vector>

#include <boost/container/small_vector.hpp>

#include <userver/proto-structs/exceptions.hpp>
#include <userver/utils/impl/internal_tag_fwd.hpp>

namespace google::protobuf {
class Descriptor;
class FieldDescriptor;
}  // namespace google::protobuf

USERVER_NAMESPACE_BEGIN

/// @brief Namespace for conversion utlities and predefined proto-structs conversion hooks for well-known std/userver
///        types.
namespace proto_structs::io {

/// @brief Read/write operation context.
class Context {
protected:
    explicit Context(const ::google::protobuf::Descriptor& message_desc) noexcept : top_message_desc_(message_desc) {}

    void PushToPath(int field_number) { path_.push_back(field_number); }
    void PopFromPath() { path_.pop_back(); }

    std::string GetCurrentPath(int current_field_number) const;
    std::string GetCurrentPath(const ::google::protobuf::FieldDescriptor& current_field_desc) const;
    std::string GetCurrentPath() const;

private:
    std::string GetCurrentPathImpl(const ::google::protobuf::Descriptor*& last_message_desc) const;

    const ::google::protobuf::Descriptor& top_message_desc_;
    ::boost::container::small_vector<int, 16> path_;
};

/// @brief Read/write operation context with error information.
/// @tparam TError error type
template <typename TError>
class ContextWithErrors : public Context {
public:
    using Error = TError;

    /// @brief Returns `true` if context contains at least one error.
    bool HasErrors() const noexcept { return !errors_.empty(); }

    /// @cond
    const std::vector<Error>& GetErrors(utils::impl::InternalTag) const& noexcept;
    std::vector<Error>&& GetErrors(utils::impl::InternalTag) && noexcept;
    /// @endcond

    /// @brief Adds conversion error.
    /// @throws Error exception.
    /// Parameter @a field_number contains protobuf message field number for which conversion has failed.
    void AddError(int field_number, std::string_view reason);

    /// @brief Adds conversion error.
    /// Parameter @a field_desc contains protobuf message field descriptor for which conversion has failed.
    void AddError(const ::google::protobuf::FieldDescriptor& field_desc, std::string_view reason);

    /// @brief Adds conversion error.
    /// Method is intended for cases when it is hard to associate error with concrete field of the protobuf message.
    void AddError(std::string_view reason);

protected:
    using Context::Context;
    using Context::GetCurrentPath;
    using Context::PopFromPath;
    using Context::PushToPath;

private:
    void AddError(std::string_view path, std::string_view reason);

    std::vector<Error> errors_;
};

extern template class ContextWithErrors<ReadError>;
extern template class ContextWithErrors<WriteError>;

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
