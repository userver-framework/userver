/**
        curl-ev: wrapper for integrating libcurl with libev applications
        Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
        See COPYING for license information.

        Native libcurl API
*/

#pragma once

#include <sys/socket.h>
#include <system_error>

namespace curl {
namespace native {

#include <curl/curl.h>
}
}  // namespace curl

inline void throw_error(std::error_code ec, const char* s) {
  if (ec) throw std::system_error(ec, s);
}

#define PP_STRINGIZE(x) #x
