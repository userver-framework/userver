#include <greeting.hpp>

#include <fmt/format.h>

#include <userver/utils/assert.hpp>

namespace service_template {

std::string SayHelloTo(std::string_view name, UserType type) {
    if (name.empty()) {
        name = "unknown user";
    }

    switch (type) {
        case UserType::kFirstTime:
            return fmt::format("Hello, {}!\n", name);
        case UserType::kKnown:
            return fmt::format("Hi again, {}!\n", name);
    }

    UASSERT(false);
}

}  // namespace service_template
