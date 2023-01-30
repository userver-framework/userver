#pragma once

#include <iterator>
#include <optional>
#include <string>
#include <vector>

#include <userver/server/http/header_map_iterator.hpp>

#include <userver/utils/checked_pointer.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace header_map_impl {
class Map;
}

struct SpecialHeader final {
  std::string_view name;
  std::size_t hash;

  explicit constexpr SpecialHeader(std::string_view name);
};

class HeaderMap final {
 public:
  using iterator = HeaderMapIterator;
  using const_iterator = HeaderMapConstIterator;

  HeaderMap();
  ~HeaderMap();
  HeaderMap(const HeaderMap& other);
  HeaderMap(HeaderMap&& other) noexcept;

  std::size_t size() const noexcept;
  bool empty() const noexcept;
  void clear();

  HeaderMapIterator find(const std::string& key);
  HeaderMapConstIterator find(const std::string& key) const;

  HeaderMapIterator find(SpecialHeader key) noexcept;
  HeaderMapConstIterator find(SpecialHeader key) const noexcept;

  void Insert(std::string key, std::string value);
  void Insert(SpecialHeader key, std::string value);
  // TODO : think about `string_view value` here
  void InsertOrAppend(std::string key, std::string value);
  void InsertOrAppend(SpecialHeader key, std::string value);

  void Erase(const std::string& key);
  void Erase(SpecialHeader key);

  HeaderMapIterator begin() noexcept;
  HeaderMapConstIterator begin() const noexcept;
  HeaderMapConstIterator cbegin() const noexcept;

  HeaderMapIterator end() noexcept;
  HeaderMapConstIterator end() const noexcept;
  HeaderMapConstIterator cend() const noexcept;

 private:
  utils::FastPimpl<header_map_impl::Map, 136, 8> impl_;
};

}  // namespace server::http

USERVER_NAMESPACE_END
