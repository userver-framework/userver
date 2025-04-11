// Copyright (c) 2016-2024 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_FOR_EACH_FIELD_HPP
#define BOOST_PFR_DETAIL_FOR_EACH_FIELD_HPP
#pragma once

#include <boost/pfr/detail/config.hpp>

#ifdef BOOST_PFR_HAS_STD_MODULE
import std;
#else
#include <type_traits>      // metaprogramming stuff
#endif

#include <boost/pfr/detail/core.hpp>
#include <boost/pfr/detail/fields_count.hpp>
#include <boost/pfr/detail/for_each_field_impl.hpp>
#include <boost/pfr/detail/make_integer_sequence.hpp>

namespace boost { namespace pfr { namespace detail {

template <class T, class F>
constexpr void for_each_field(T&& value, F&& func) {
    constexpr std::size_t fields_count_val = boost::pfr::detail::fields_count<std::remove_reference_t<T>>();

    ::boost::pfr::detail::for_each_field_dispatcher(
        value,
        [f = std::forward<F>(func)](auto&& t) mutable {
            // MSVC related workaround. Its lambdas do not capture constexprs.
            constexpr std::size_t fields_count_val_in_lambda
                = boost::pfr::detail::fields_count<std::remove_reference_t<T>>();

            ::boost::pfr::detail::for_each_field_impl(
                t,
                std::forward<F>(f),
                detail::make_index_sequence<fields_count_val_in_lambda>{},
                std::is_rvalue_reference<T&&>{}
            );
        },
        detail::make_index_sequence<fields_count_val>{}
    );
}

}}} // namespace boost::pfr::detail


#endif // BOOST_PFR_DETAIL_FOR_EACH_FIELD_HPP
