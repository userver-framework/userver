#pragma once

/// @file userver/utils/datetime/cpp_20_calendar.hpp
/// @brief This file brings date.h into utils::datetime::date namespace,
/// which is std::chrono::operator/ (calendar) in c++20.
/// @ingroup userver_universal

// TODO : replace with C++20 std::chrono:: when time comes

#include <date/date.h>

USERVER_NAMESPACE_BEGIN

namespace utils::datetime {

namespace date = ::date;

}

USERVER_NAMESPACE_END
