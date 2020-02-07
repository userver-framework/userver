#pragma once

#include <string>
#include <unordered_map>
#include <utils/projecting_view.hpp>
#include <utils/str_icase.hpp>

namespace server::http {

using HeadersMap =
    std::unordered_map<std::string, std::string, utils::StrIcaseHash,
                       utils::StrIcaseEqual>;
using CookiesMap = HeadersMap;

using HeadersMapKeys = decltype(utils::MakeKeysView(HeadersMap{}));
using CookiesMapKeys = decltype(utils::MakeKeysView(CookiesMap{}));

}  // namespace server::http
