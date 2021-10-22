/** @file curl-ev/string_list.hpp
        curl-ev: wrapper for integrating libcurl with libev applications
        Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
        See COPYING for license information.

        Constructs libcurl string lists
*/

#pragma once

#include <deque>
#include <optional>
#include <string>
#include <string_view>

#include "native.hpp"

USERVER_NAMESPACE_BEGIN

namespace curl {

class string_list {
 public:
  string_list() = default;
  string_list(const string_list&) = delete;
  string_list(string_list&&) = delete;
  string_list& operator=(const string_list&) = delete;
  string_list& operator=(string_list&&) = delete;

  inline native::curl_slist* native_handle() {
    return list_elements_.empty() ? nullptr : &list_elements_.front().list_node;
  }

  inline const native::curl_slist* native_handle() const {
    return list_elements_.empty() ? nullptr : &list_elements_.front().list_node;
  }

  void add(std::string str);
  void clear() noexcept;

  template <typename Pred>
  std::optional<std::string_view> FindIf(const Pred& pred) const {
    for (const auto& list_elem : list_elements_) {
      const auto& value = list_elem.value;
      if (pred(value)) return value;
    }
    return std::nullopt;
  }

  template <typename Pred>
  bool ReplaceFirstIf(const Pred& pred, std::string&& new_value) {
    for (auto& list_elem : list_elements_) {
      const auto& value = list_elem.value;
      if (pred(value)) {
        ReplaceValue(list_elem, std::move(new_value));
        return true;
      }
    }
    return false;
  }

 private:
  struct Elem {
    explicit Elem(std::string new_value);

    std::string value{};
    native::curl_slist list_node{};
  };

  static void ReplaceValue(Elem& list_elem, std::string&& new_value);

  std::deque<Elem> list_elements_;
};

}  // namespace curl

USERVER_NAMESPACE_END
