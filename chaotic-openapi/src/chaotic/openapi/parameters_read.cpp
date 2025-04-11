#include <userver/chaotic/openapi/parameters_read.hpp>

#include <stdexcept>

#include <boost/algorithm/string/split.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi {

namespace impl {

void SplitByDelimiter(std::string_view str, char delimiter, utils::function_ref<void(std::string)> func) {
    for (auto it = boost::algorithm::make_split_iterator(str, boost::token_finder([delimiter](char c) {
                                                             return c == delimiter;
                                                         }));
         it != decltype(it){};
         ++it) {
        func(std::string{it->begin(), it->end()});
    }
}

}  // namespace impl

std::string FromStr(std::string&& str_value, parse::To<std::string>) { return std::move(str_value); }

bool FromStr(std::string&& str_value, parse::To<bool>) {
    if (str_value == "true") return true;
    if (str_value == "false") return false;
    throw std::runtime_error("Unknown bool value: " + str_value);
}

double FromStr(std::string&& str_value, parse::To<double>) { return utils::FromString<double>(str_value); }

}  // namespace chaotic::openapi

USERVER_NAMESPACE_END
