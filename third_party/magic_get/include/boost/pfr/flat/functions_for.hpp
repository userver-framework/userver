// Copyright (c) 2016-2020 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_FLAT_FUNCTIONS_FOR_HPP
#define BOOST_PFR_FLAT_FUNCTIONS_FOR_HPP
#pragma once

#include <boost/pfr/detail/config.hpp>

#include <boost/pfr/flat/functors.hpp>
#include <boost/pfr/flat/io.hpp>

/// \def BOOST_PFR_FLAT_FUNCTIONS_FOR(T)
/// Defines comparison operators and stream operators for T.
/// If POD is comparable or streamable using it's own operator (but not it's conversion operator), then the original operator is used.
///
/// \b Example:
/// \code
///     #include <boost/pfr/flat/functions_for.hpp>
///     struct comparable_struct {      // No operators defined for that structure
///         int i; short s; char data[7]; bool bl; int a,b,c,d,e,f;
///     };
///     BOOST_PFR_FLAT_FUNCTIONS_FOR(comparable_struct)
///     // ...
///
///     comparable_struct s1 {0, 1, "Hello", false, 6,7,8,9,10,11};
///     comparable_struct s2 {0, 1, "Hello", false, 6,7,8,9,10,11111};
///     assert(s1 < s2);
///     std::cout << s1 << std::endl; // Outputs: {0, 1, H, e, l, l, o, , , 0, 6, 7, 8, 9, 10, 11}
/// \endcode
///
/// \rcast
///
/// \podops for other ways to define operators and more details.
///
/// \b Defines \b following \b for \b T:
/// \code
/// bool operator==(const T& lhs, const T& rhs) noexcept;
/// bool operator!=(const T& lhs, const T& rhs) noexcept;
/// bool operator< (const T& lhs, const T& rhs) noexcept;
/// bool operator> (const T& lhs, const T& rhs) noexcept;
/// bool operator<=(const T& lhs, const T& rhs) noexcept;
/// bool operator>=(const T& lhs, const T& rhs) noexcept;
///
/// template <class Char, class Traits>
/// std::basic_ostream<Char, Traits>& operator<<(std::basic_ostream<Char, Traits>& out, const T& value);
///
/// template <class Char, class Traits>
/// std::basic_istream<Char, Traits>& operator>>(std::basic_istream<Char, Traits>& in, T& value);
///
/// // helper function for Boost unordered containers and boost::hash<>.
/// std::size_t hash_value(const T& value) noexcept;
/// \endcode

#define BOOST_PFR_FLAT_FUNCTIONS_FOR(T)                                                                                                                    \
    BOOST_PFR_MAYBE_UNUSED static inline bool operator==(const T& lhs, const T& rhs) noexcept { return ::boost::pfr::flat_equal_to<T>{}(lhs, rhs);      }  \
    BOOST_PFR_MAYBE_UNUSED static inline bool operator!=(const T& lhs, const T& rhs) noexcept { return ::boost::pfr::flat_not_equal<T>{}(lhs, rhs);     }  \
    BOOST_PFR_MAYBE_UNUSED static inline bool operator< (const T& lhs, const T& rhs) noexcept { return ::boost::pfr::flat_less<T>{}(lhs, rhs);          }  \
    BOOST_PFR_MAYBE_UNUSED static inline bool operator> (const T& lhs, const T& rhs) noexcept { return ::boost::pfr::flat_greater<T>{}(lhs, rhs);       }  \
    BOOST_PFR_MAYBE_UNUSED static inline bool operator<=(const T& lhs, const T& rhs) noexcept { return ::boost::pfr::flat_less_equal<T>{}(lhs, rhs);    }  \
    BOOST_PFR_MAYBE_UNUSED static inline bool operator>=(const T& lhs, const T& rhs) noexcept { return ::boost::pfr::flat_greater_equal<T>{}(lhs, rhs); }  \
    template <class Char, class Traits>                                                                                                                    \
    BOOST_PFR_MAYBE_UNUSED static ::std::basic_ostream<Char, Traits>& operator<<(::std::basic_ostream<Char, Traits>& out, const T& value) {                \
        ::boost::pfr::flat_write(out, value);                                                                                                              \
        return out;                                                                                                                                        \
    }                                                                                                                                                      \
    template <class Char, class Traits>                                                                                                                    \
    BOOST_PFR_MAYBE_UNUSED static ::std::basic_istream<Char, Traits>& operator>>(::std::basic_istream<Char, Traits>& in, T& value) {                       \
        ::boost::pfr::flat_read(in, value);                                                                                                                \
        return in;                                                                                                                                         \
    }                                                                                                                                                      \
    BOOST_PFR_MAYBE_UNUSED static inline std::size_t hash_value(const T& v) noexcept {                                                                     \
        return ::boost::pfr::flat_hash<T>{}(v);                                                                                                            \
    }                                                                                                                                                      \
/**/

#endif // BOOST_PFR_FLAT_FUNCTIONS_FOR_HPP


