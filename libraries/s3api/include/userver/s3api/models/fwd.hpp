#pragma once

// secret is strong typedef, so we can't just forward declare it :(
#include <userver/s3api/models/secret.hpp>

USERVER_NAMESPACE_BEGIN

namespace s3api {

struct Request;

}  // namespace s3api

USERVER_NAMESPACE_END
