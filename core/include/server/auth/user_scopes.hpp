#pragma once

#include <functional>
#include <string>
#include <vector>

#include <formats/parse/to.hpp>
#include <utils/string_view.hpp>

namespace server {
namespace auth {

class UserScope {
 public:
  explicit UserScope(std::string value) : value_(std::move(value)) {}

  const std::string& GetValue() const { return value_; }

 private:
  std::string value_;
};

inline bool operator==(const UserScope& lhs, const UserScope& rhs) {
  return lhs.GetValue() == rhs.GetValue();
}

inline bool operator!=(const UserScope& lhs, const UserScope& rhs) {
  return lhs.GetValue() != rhs.GetValue();
}

inline bool operator==(const UserScope& lhs, utils::string_view rhs) {
  return lhs.GetValue() == rhs;
}

inline bool operator==(utils::string_view lhs, const UserScope& rhs) {
  return lhs == rhs.GetValue();
}

inline bool operator!=(const UserScope& lhs, utils::string_view rhs) {
  return lhs.GetValue() != rhs;
}

inline bool operator!=(utils::string_view lhs, const UserScope& rhs) {
  return lhs == rhs.GetValue();
}

template <class Value>
UserScope Parse(const Value& v, formats::parse::To<UserScope>) {
  return UserScope{v.template As<std::string>()};
}

using UserScopes = std::vector<UserScope>;

}  // namespace auth
}  // namespace server

namespace std {
template <>
struct hash<server::auth::UserScope> {
  std::size_t operator()(const server::auth::UserScope& v) const {
    return std::hash<std::string>{}(v.GetValue());
  }
};
}  // namespace std
