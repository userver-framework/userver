#pragma once

/// @file userver/utils/strerror.hpp
/// @brief /// MT-safe version of POSIX functions ::strerror and ::strsignal

#include <string>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// MT-safe version of POSIX function ::strerror
std::string strerror(int return_code);

/// MT-safe version of POSIX function ::strsignal
std::string strsignal(int signal_num);

}  // namespace utils

USERVER_NAMESPACE_END
