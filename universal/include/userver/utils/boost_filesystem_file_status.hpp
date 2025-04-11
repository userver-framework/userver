#pragma once

#include <boost/version.hpp>

#if BOOST_VERSION >= 107200
// Compilation speed-up
#include <boost/filesystem/file_status.hpp>
#else
#include <boost/filesystem/operations.hpp>
#endif
