#include <userver/clients/http/cookie_jar.hpp>

#include <atomic>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

CookieJar::CookieJar(std::vector<std::string>&& cookies) : 
    cookies_(std::move(cookies)) {
}

std::optional<std::string> CookieJar::FindCookieValue(std::string_view name) const {
    //  TODO: not optimal way to search
    for (std::string_view line : cookies_) {
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
            line.remove_suffix(1);
        }

        const auto value_pos = line.rfind('\t');
        if (value_pos == std::string_view::npos || value_pos == 0) {
            // A comment or a malformed line, both have nothing to look at.
            continue;
        }
        const auto name_pos = line.rfind('\t', value_pos - 1);
        if (name_pos == std::string_view::npos) {
            continue;
        }

        // Cookie names are case-sensitive, RFC 6265, section 5.3.
        if (line.substr(name_pos + 1, value_pos - name_pos - 1) == name) {
            return std::string{line.substr(value_pos + 1)};
        }
    }
    return std::nullopt;
}

}  // namespace clients::http

USERVER_NAMESPACE_END

#if defined(__linux__) && !defined(USERVER_IMPL_STATIC_CURL)
/*
    Dynamically linked curl and libpsl can use filesystem in order to get fresh prefix graph for PUBLIC SUFFIX LIST.
    This check is performed at dangerous place, at receiving of cookie on libev thread.
    Internally psl graph is cached per easy handle for up to 72 hours. 
    Algorithm to temporary cope (until is new curl's api is provided) is following:
        1) For statically linked curl OR no linux based api we are supposed that psl is turned off.
        2) For dynamically linked curl and dynamically linked psl we are trying to skip fs operating by injecting `psl_latest` method
        3) For dynamically linked curl with statically linked psl we cannot do anything until new curl's api. Test should catch this tricky curl
*/

#include <dlfcn.h>

struct psl_ctx_st;

namespace {

//  Counter of invocation of injected method. Useful for testing whether injection is working
std::atomic<std::size_t> psl_latest_calls{0};

}  // namespace

extern "C" {

#ifndef __clang__
[[gnu::visibility("default")]] [[gnu::externally_visible]]
#endif
psl_ctx_st* psl_latest(const char*) {
    psl_latest_calls.fetch_add(1, std::memory_order_relaxed);
    //  Internally `psl_latest` goes through list of predefined locations to load dataset from fs with builtin fallback
    //  Here are we skip fs lookup to builtin directly
    static auto func = reinterpret_cast<const psl_ctx_st* (*)()>(dlsym(RTLD_DEFAULT, "psl_builtin"));
    if (func == nullptr) return nullptr;
    return const_cast<psl_ctx_st*>(func());
}

//  Note: overriding free is not needed. By default it does not release builtin context

//  Gets counter of invocations of injected `psl_latest`, useful for tests
std::size_t userver_impl_psl_latest_calls() { return psl_latest_calls.load(std::memory_order_relaxed); }
}

#endif
