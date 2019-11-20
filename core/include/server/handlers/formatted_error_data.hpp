#pragma once

#include <string>

#include <boost/optional.hpp>

#include <http/content_type.hpp>

namespace server {
namespace handlers {

struct FormattedErrorData {
  std::string external_body;
  boost::optional<::http::ContentType> content_type{};
};

}  // namespace handlers
}  // namespace server
