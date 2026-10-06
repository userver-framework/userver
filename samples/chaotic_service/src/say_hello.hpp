#pragma once

#include <schemas/hello.hpp>

namespace samples::hello {

HelloResponseBody SayHelloTo(const HelloRequestBody&);

}  // namespace samples::hello
