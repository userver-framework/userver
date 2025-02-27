#pragma once

#include <string>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace kafka {

struct Header {
    std::string name;
    std::string value;
};

using Headers = std::vector<Header>;

}  // namespace kafka

USERVER_NAMESPACE_END
