#pragma once

/// @file userver/protobuf/log.hpp

#include <type_traits>

#include <google/protobuf/message.h>

#include <userver/logging/log_helper.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

/// @brief Logs the protobuf @a message as a debug string (see @ref protobuf::json::MessageToDebugString).
logging::LogHelper& operator<<(logging::LogHelper& h, const google::protobuf::Message& message);

/// @brief Logs @a message — any protobuf message type, derived from or equal to `google::protobuf::Message` — as
/// a debug string, forwarding to the overload above.
///
/// This has to be a *constrained* template rather than an unconstrained one: for a concrete derived message type,
/// `LogHelper`'s own generic member `operator<<(const T&)` is an exact match, while binding straight to
/// `const google::protobuf::Message&` requires a derived-to-base conversion — so an unconstrained free function
/// would always lose to the member. Deducing @a T directly gives this overload an equally exact match; C++20
/// partial ordering then prefers the more constrained candidate (this one) over the member's unconstrained
/// template.
///
/// @warning The constraint must actually restrict @a T to `google::protobuf::Message` and its derived types.
/// `std::is_base_of_v<::google::protobuf::Message, T>` alone is both necessary and sufficient (it is already
/// `true` for `T = google::protobuf::Message` itself, since `is_base_of` treats a type as its own base). Do not
/// widen it (e.g. by `|| !std::is_same_v<...>`, as used for @ref JsonToMessage, where it is vestigial but
/// harmless): unlike `JsonToMessage`, this template competes with `LogHelper`'s unconstrained member
/// `operator<<`, and C++20 partial ordering picks the *more constrained* candidate without evaluating whether the
/// constraint is actually restrictive — a tautological constraint (`true` for every `T`) would still "win", then
/// hijack `h << x` for values unrelated to protobuf (e.g. plain `std::string`, as produced by
/// @ref protobuf::json::MessageToDebugString below) and fail to compile on the `static_cast` in the body.
///
/// The `static_cast` in the body changes the argument's static type to exactly `google::protobuf::Message`, so on
/// the resulting `h << ...` call the non-template overload above — preferred by the standard tie-breaking rule
/// over an equally exact-match template — is the one actually selected, instead of recursing back into this
/// template (or, worse, back into the member).
template <typename T>
requires(std::is_base_of_v<::google::protobuf::Message, T>)
logging::LogHelper& operator<<(logging::LogHelper& h, const T& message) {
    return h << static_cast<const google::protobuf::Message&>(message);
}

}  // namespace logging

USERVER_NAMESPACE_END
