#pragma once

#include <userver/http/header_map.hpp>

#include <http/header_map/map.hpp>

USERVER_NAMESPACE_BEGIN

namespace http::headers {

class TestsHelper final {
 public:
  static header_map::Map& GetMapImpl(HeaderMap& map);

  static const header_map::Danger& GetMapDanger(const HeaderMap& map);

  static std::size_t GetMapMaxDisplacement(const HeaderMap& map);

  static void ForceIntoRedState(HeaderMap& map);
};

}  // namespace http::headers

USERVER_NAMESPACE_END
