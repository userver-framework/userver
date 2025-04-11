#pragma once

#include <cstddef>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace server::request {

class RequestParser {
public:
    RequestParser() = default;
    RequestParser(RequestParser&&) = delete;
    RequestParser& operator=(RequestParser&&) = delete;

    virtual ~RequestParser() noexcept = default;

    virtual bool Parse(std::string_view request) = 0;
};

}  // namespace server::request

USERVER_NAMESPACE_END
