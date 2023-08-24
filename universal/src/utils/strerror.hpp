#pragma once

#include <string>

USERVER_NAMESPACE_BEGIN

namespace utils {

/* MT-safe versions of POSIX functions strerror, strsignal */

std::string strerror(int return_code);

std::string strsignal(int signal_num);

}  // namespace utils

USERVER_NAMESPACE_END
