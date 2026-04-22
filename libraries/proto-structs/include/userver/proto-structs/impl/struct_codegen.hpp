#pragma once

#include <concepts>

#include <boost/pfr/ops_fields.hpp>

#define UPROTO_DEFINE_OPERATOR_EQUALS(struct_name)  \
    template <typename Other>                       \
    requires std::same_as<struct_name, Other>       \
    bool operator==(const Other& other) const {     \
        return boost::pfr::eq_fields(*this, other); \
    }
