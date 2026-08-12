#pragma once

/// @file userver/utest/literals.hpp
/// @brief Brings format literals into scope for unit tests.
/// @ingroup userver_universal

USERVER_NAMESPACE_BEGIN

namespace formats::literals {}

USERVER_NAMESPACE_END

// NOLINTNEXTLINE(google-build-using-namespace, google-global-names-in-headers)
using namespace USERVER_NAMESPACE::formats::literals;
