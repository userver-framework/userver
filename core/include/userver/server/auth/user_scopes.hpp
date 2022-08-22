#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include <userver/formats/parse/to.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::auth {

class UserScope final {
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

inline bool operator==(const UserScope& lhs, std::string_view rhs) {
  return lhs.GetValue() == rhs;
}

inline bool operator==(std::string_view lhs, const UserScope& rhs) {
  return lhs == rhs.GetValue();
}

inline bool operator!=(const UserScope& lhs, std::string_view rhs) {
  return lhs.GetValue() != rhs;
}

inline bool operator!=(std::string_view lhs, const UserScope& rhs) {
  return lhs != rhs.GetValue();
}

template <class Value>
UserScope Parse(const Value& v, formats::parse::To<UserScope>) {
  return UserScope{v.template As<std::string>()};
}

using UserScopes = std::vector<UserScope>;

}  // namespace server::auth

USERVER_NAMESPACE_END

namespace std {
template <>
struct hash<USERVER_NAMESPACE::server::auth::UserScope> {
  std::size_t operator()(
      const USERVER_NAMESPACE::server::auth::UserScope& v) const {
    return std::hash<std::string>{}(v.GetValue());
  }
};
}  // namespace std
