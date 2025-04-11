#pragma once

/// @file userver/s3api/clients/fwd.hpp
/// @brief Forward declarations

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace s3api {

class Client;
using ClientPtr = std::shared_ptr<Client>;

}  // namespace s3api

USERVER_NAMESPACE_END
