#pragma once

#include <string>
#include <string_view>

#include <schemas/hello.hpp>

namespace samples::hello {

HelloResponseBody SayHelloTo(const HelloRequestBody&);

}  // namespace samples::hello
