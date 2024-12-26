#include "say_hello.hpp"

#include <fmt/format.h>

namespace samples::hello {

std::string SayHelloTo(std::string_view name) {
    if (name.empty()) {
        name = "unknown user";
    }

    return fmt::format("Hello, {}!\n", name);
}

}  // namespace samples::hello
