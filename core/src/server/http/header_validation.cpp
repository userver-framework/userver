#include "header_validation.hpp"

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace server::http {

void CheckHeaderName(std::string_view name) {
    static constexpr auto init = []() {
        std::array<uint8_t, 256> res{};  // zero initialize
        for (int i = 0; i < 32; i++) {
            res[i] = 1;
        }
        for (int i = 127; i < 256; i++) {
            res[i] = 1;
        }
        for (const unsigned char c : "()<>@,;:\\\"/[]?={} \t") {
            res[c] = 1;
        }
        return res;
    };
    static constexpr auto bad_chars = init();

    bool check_failed = false;

    // this gets autovectorized, and we optimize for happy path here
    for (const char c : name) {
        const auto code = static_cast<uint8_t>(c);
        check_failed |= bad_chars[code];
    }

    if (check_failed) {
        // in a presumably rare scenarios of the check failing we do a second loop
        for (const char c : name) {
            const auto code = static_cast<uint8_t>(c);
            if (bad_chars[code]) {
                throw std::runtime_error(
                    fmt::format("invalid character in header name: '{}' (#{}), full header name: {}", c, code, name)
                );
            }
        }
    }
}

void CheckHeaderValue(std::string_view value) {
    bool check_failed = false;

    // this gets autovectorized, and we optimize for happy path here
    for (const char c : value) {
        auto code = static_cast<uint8_t>(c);
        check_failed |= code < 32 || code == 127;
    }

    if (check_failed) {
        // in a presumably rare scenarios of the check failing we do a second loop
        for (const char c : value) {
            auto code = static_cast<uint8_t>(c);
            if (code < 32 || code == 127) {
                throw std::runtime_error(
                    std::string("invalid character in header value: '") + c + "' (#" + std::to_string(code) + ")"
                );
            }
        }
    }
}

}  // namespace server::http

USERVER_NAMESPACE_END
