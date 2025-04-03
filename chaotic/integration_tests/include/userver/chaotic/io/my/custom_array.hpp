#pragma once

#include <set>
#include <vector>

#include <userver/chaotic/convert/to.hpp>

namespace my {

template <typename T>
struct CustomArray final {
    template <typename U>
    CustomArray(const U& value) : s(value.begin(), value.end()) {}
    std::vector<T> s;
};

template <typename T>
bool operator==(const CustomArray<T>& lhs, const CustomArray<T>& rhs) {
    return lhs.s == rhs.s;
}

template <typename T>
std::vector<T> Convert(const CustomArray<T>& value, USERVER_NAMESPACE::chaotic::convert::To<std::vector<T>>) {
    return value.s;
}

template <typename T>
std::set<T> Convert(const CustomArray<T>& value, USERVER_NAMESPACE::chaotic::convert::To<std::set<T>>) {
    return {value.s.begin(), value.s.end()};
}

}  // namespace my
