#pragma once

/// @file userver/utils/overloaded.hpp
/// @brief @copybrief utils::Overloaded

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @brief Utility to define std::variant visitors in a simple way
template <class... Ts>
struct Overloaded : Ts... {  // NOLINT(fuchsia-multiple-inheritance)
  using Ts::operator()...;
};
template <class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

}  // namespace utils

USERVER_NAMESPACE_END
