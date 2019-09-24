#pragma once

#include <string>
#include <unordered_map>
#include <utils/str_icase.hpp>

#include <boost/range/adaptor/map.hpp>

namespace server {
namespace http {

using HeadersMap =
    std::unordered_map<std::string, std::string, utils::StrIcaseHash,
                       utils::StrIcaseEqual>;
using CookiesMap = HeadersMap;

using HeadersMapKeys = decltype(HeadersMap() | boost::adaptors::map_keys);
using CookiesMapKeys = decltype(CookiesMap() | boost::adaptors::map_keys);

}  // namespace http
}  // namespace server
