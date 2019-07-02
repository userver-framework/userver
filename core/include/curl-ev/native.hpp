/** @file curl-ev/native.hpp
        curl-ev: wrapper for integrating libcurl with libev applications
        Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
        See COPYING for license information.

        Native libcurl API
*/

#pragma once

/* List of system headers used by libcurl 7.58:
 *
 * include/curl$ grep -r '#include <' -h | sed 's!\s*\s/\*.*!!' | sort -u
 *
 * Explicitly include them before <curl/curl.h> as the latter
 * is included in curl::native namespace. If any system header
 * is included as part of <curl/curl.h>, it will be unavailable
 * to the rest of the world.
 */
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

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
