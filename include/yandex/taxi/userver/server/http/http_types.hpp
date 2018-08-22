#pragma once

#include <yandex/taxi/userver/utils/str_icase.hpp>
#include <string>
#include <unordered_map>

#include <boost/range/adaptor/map.hpp>


namespace server {
namespace http {

  using HeadersMap =
      std::unordered_map<std::string, std::string, utils::StrIcaseHash,
                         utils::StrIcaseCmp>;
  using CookiesMap = HeadersMap;

  using HeadersMapKeys = decltype(HeadersMap() | boost::adaptors::map_keys);
  using CookiesMapKeys = decltype(CookiesMap() | boost::adaptors::map_keys);

}
}
