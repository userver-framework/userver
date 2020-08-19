#pragma once

namespace utils {

/// Utility to define std::variant visitors in a simple way
template <class... Ts>
struct Overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
Overloaded(Ts...)->Overloaded<Ts...>;

}  // namespace utils
