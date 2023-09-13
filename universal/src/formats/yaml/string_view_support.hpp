#pragma once

#include <string_view>
#include <type_traits>

#include <yaml-cpp/yaml.h>

#include <userver/utils/assert.hpp>

// yaml-cpp library added convert<std::string_view> specialization at some
// point. We have to support old and new versions of the library, so the
// following workaround is required:

USERVER_NAMESPACE_BEGIN

namespace impl {

template <class T>
constexpr auto IsSpecialized() -> decltype(sizeof(T), std::true_type{}) {
  return {};
}

template <class T, class... Args>
constexpr std::false_type IsSpecialized(const Args&...) {
  return {};
}

struct SomeFake {
  explicit operator std::string() const noexcept {
    UASSERT_MSG(false,
                "YAML specialization of convert<std::string_view> is broken. "
                "Contact the userver maintainers");
    return {};
  }

  template <class T>
  SomeFake& operator=(const T&) {
    UASSERT_MSG(false,
                "YAML specialization of convert<std::string_view> is broken. "
                "Contact the userver maintainers");
    return *this;
  }
};

using StringViewOrSomeFake =
    std::conditional_t<impl::IsSpecialized<::YAML::convert<std::string_view>>(),
                       SomeFake, std::string_view>;

}  // namespace impl

USERVER_NAMESPACE_END

namespace YAML {

// Makes YAML work with std::string_view as keys
template <>
struct convert<USERVER_NAMESPACE::impl::StringViewOrSomeFake> {
  using ConversionType = USERVER_NAMESPACE::impl::StringViewOrSomeFake;

  static Node encode(ConversionType rhs) { return Node(std::string{rhs}); }

  static bool decode(const Node& node, ConversionType& rhs) {
    if (!node.IsScalar()) return false;
    rhs = node.Scalar();
    return true;
  }
};

}  // namespace YAML
