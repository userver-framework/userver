#pragma once

/// @file userver/utils/boost_filesystem_file_status.hpp
/// @brief Lightweight include of Boost.Filesystem file_status.
/// @ingroup userver_universal

#include <boost/version.hpp>

#if BOOST_VERSION >= 107200
// Compilation speed-up
#include <boost/filesystem/file_status.hpp>
#else
#include <boost/filesystem/operations.hpp>
#endif
