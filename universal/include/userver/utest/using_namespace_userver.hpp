#pragma once

/// @file userver/utest/using_namespace_userver.hpp
/// @brief For samples and snippets only! Has a `using namespace
/// USERVER_NAMESPACE;` if the USERVER_NAMESPACE is not empty.

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IS_EMPTY_MACRO_HELPER(X) X##1

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IS_EMPTY_MACRO(X) (USERVER_IS_EMPTY_MACRO_HELPER(X) == 1)

#if !USERVER_IS_EMPTY_MACRO(USERVER_NAMESPACE)

USERVER_NAMESPACE_BEGIN
USERVER_NAMESPACE_END

// NOLINTNEXTLINE(google-build-using-namespace, google-global-names-in-headers)
using namespace USERVER_NAMESPACE;

#endif

#undef USERVER_IS_EMPTY_MACRO
#undef USERVER_IS_EMPTY_MACRO_HELPER
