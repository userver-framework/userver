#pragma once

/// @file userver/proto-structs/unbreakable_dependency_cycle.hpp
/// @brief @copybrief proto_structs::UnbreakableDependencyCycle

USERVER_NAMESPACE_BEGIN

namespace proto_structs {

/// @brief A type that is generated for a field that participates in an unbreakable dependency cycle.
///
/// For example:
///
/// @code
/// struct A {
///     struct Inner {
///         B::Inner field;
///     };
/// };
///
/// struct B {
///     struct Inner {
///         A::Inner field;
///     };
/// };
/// @endcode
///
/// Nested types cannot be forwarded and wrapped in @ref utils::Box, so we give up.
/// At least we generate something valid for other fields and types in the file,
/// so that if the current field is not actually needed, then proto structs are usable.
struct UnbreakableDependencyCycle final {
    /// We cannot actually compare the contents of such field, because they are not parsed into proto structs.
    /// Transitively, any data structure containing such field is not comparable.
    bool operator==(const UnbreakableDependencyCycle& other) const = delete;
};

}  // namespace proto_structs

USERVER_NAMESPACE_END
