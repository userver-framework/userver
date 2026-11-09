#pragma once

#include <bitset>
#include <cstddef>
#include <initializer_list>

#include <userver/http/status_code.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

/// @brief Owning allocation-free set of explicitly expected HTTP status codes.
class ExpectedStatusCodes final {
public:
    constexpr ExpectedStatusCodes() = default;
    /// @throws std::invalid_argument if a status code is outside [100, 599].
    ExpectedStatusCodes(std::initializer_list<USERVER_NAMESPACE::http::StatusCode> status_codes);

    constexpr ExpectedStatusCodes(const ExpectedStatusCodes&) = default;
    constexpr ExpectedStatusCodes& operator=(const ExpectedStatusCodes&) = default;

    bool Contains(USERVER_NAMESPACE::http::StatusCode status_code) const;
    /// @brief Add or remove a status code from the set.
    /// @throws std::invalid_argument if a status code is outside [100, 599]; the set is unchanged.
    void Set(USERVER_NAMESPACE::http::StatusCode status_code, bool expected);
    bool operator==(const ExpectedStatusCodes&) const = default;

private:
    static constexpr int kMinHttpStatusCode = 100;
    static constexpr int kMaxHttpStatusCode = 599;
    static constexpr std::size_t kStatusCodesCount = kMaxHttpStatusCode - kMinHttpStatusCode + 1;

    std::bitset<kStatusCodesCount> status_codes_{};
};

}  // namespace clients::http

USERVER_NAMESPACE_END
