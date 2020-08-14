// Copyright (c) 2019-2020 Antony Polukhin.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_UNSAFE_DECLVAL_HPP
#define BOOST_PFR_DETAIL_UNSAFE_DECLVAL_HPP

#include <boost/pfr/detail/config.hpp>

#include <type_traits>

namespace boost { namespace pfr { namespace detail {

// For returning non default constructible types. Never used at runtime! GCC's
// std::declval may not be used in potentionally evaluated contexts, so it does not work here.
template <class T> constexpr T unsafe_declval() noexcept {
    typename std::remove_reference<T>::type* ptr = 0;
    ptr += 42; // killing 'null pointer dereference' heuristics of static analysis tools
    return static_cast<T>(*ptr);
}

}}} // namespace boost::pfr::detail


#endif // BOOST_PFR_DETAIL_UNSAFE_DECLVAL_HPP

