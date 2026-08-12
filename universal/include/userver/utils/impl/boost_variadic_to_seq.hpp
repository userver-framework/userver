#pragma once

#include <boost/preprocessor/facilities/is_empty_variadic.hpp>
#include <boost/preprocessor/seq/seq.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>

/// @cond

// Workaround for BOOST_PP_VARIADIC_TO_SEQ failing to handle the 0-arg case.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_VARIADIC_TO_SEQ(...) \
    BOOST_PP_IF(BOOST_PP_IS_EMPTY(__VA_ARGS__), BOOST_PP_SEQ_NIL, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

/// @endcond
