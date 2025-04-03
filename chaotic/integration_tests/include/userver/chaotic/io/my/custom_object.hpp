#pragma once

#include <string>

#include <userver/chaotic/convert.hpp>

namespace ns {
struct CustomObject;
}

namespace my {

struct CustomObject {
    CustomObject() = default;

    CustomObject(const ns::CustomObject&);

    explicit CustomObject(std::string&& foo) : foo(std::move(foo)) {}

    std::string foo;
};

inline bool operator==(const CustomObject& a, const CustomObject& b) { return a.foo == b.foo; }

ns::CustomObject Convert(const CustomObject& value, USERVER_NAMESPACE::chaotic::convert::To<ns::CustomObject>);

}  // namespace my
