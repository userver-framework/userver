/**
        curl-ev: wrapper for integrating libcurl with libev applications
        Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
        See COPYING for license information.

        C++ wrapper for libcurl's string list interface
*/

#include <curl-ev/string_list.hpp>

USERVER_NAMESPACE_BEGIN

namespace curl {

string_list::Elem::Elem(std::string new_value) : value(std::move(new_value)) {
  list_node.data = value.data();
  list_node.next = nullptr;
}

void string_list::add(std::string str) {
  native::curl_slist* prev =
      list_elements_.empty() ? nullptr : &list_elements_.back().list_node;
  auto& last = list_elements_.emplace_back(std::move(str));
  if (prev) prev->next = &last.list_node;
}

void string_list::clear() noexcept { list_elements_.clear(); }

void string_list::ReplaceValue(Elem& list_elem, std::string&& new_value) {
  list_elem.value = std::move(new_value);
  list_elem.list_node.data = list_elem.value.data();
}

}  // namespace curl

USERVER_NAMESPACE_END
