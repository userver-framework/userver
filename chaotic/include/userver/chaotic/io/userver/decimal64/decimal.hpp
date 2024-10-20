#pragma once

#include <userver/chaotic/convert.hpp>
#include <userver/decimal64/decimal64.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::convert {

template <int Prec, typename RoundPolicy>
std::string Convert(const decimal64::Decimal<Prec, RoundPolicy>& value, chaotic::convert::To<std::string>) {
    return ToString(value);
}

template <int Prec, typename RoundPolicy>
decimal64::Decimal<Prec, RoundPolicy>
Convert(const std::string& str, chaotic::convert::To<decimal64::Decimal<Prec, RoundPolicy>>) {
    return decimal64::Decimal<Prec, RoundPolicy>(str);
}

template <int Prec, typename RoundPolicy>
decimal64::Decimal<Prec, RoundPolicy>
Convert(const int& str, chaotic::convert::To<decimal64::Decimal<Prec, RoundPolicy>>) {
    return decimal64::Decimal<Prec, RoundPolicy>(str);
}

template <int Prec, typename RoundPolicy>
int Convert(const decimal64::Decimal<Prec, RoundPolicy>& value, chaotic::convert::To<int>) {
    return value.ToInteger();
}

}  // namespace chaotic::convert

USERVER_NAMESPACE_END
