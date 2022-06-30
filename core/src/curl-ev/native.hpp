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
// NOLINTNEXTLINE(modernize-deprecated-headers)
#include <limits.h>
// NOLINTNEXTLINE(modernize-deprecated-headers)
#include <stdarg.h>
// NOLINTNEXTLINE(modernize-deprecated-headers)
#include <stdio.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
// NOLINTNEXTLINE(modernize-deprecated-headers)
#include <time.h>

#include <cinttypes>
#include <system_error>

USERVER_NAMESPACE_BEGIN

namespace curl::native {

#include <curl/curl.h>

}  // namespace curl::native

inline void throw_error(std::error_code ec, const char* s) {
  if (ec) throw std::system_error(ec, s);
}

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define PP_STRINGIZE(x) #x
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define PP_CONCAT2(a, b) a##b
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define PP_CONCAT3(a, b, c) a##b##c

USERVER_NAMESPACE_END
