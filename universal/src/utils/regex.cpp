#include <userver/utils/regex.hpp>

#include <atomic>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <variant>

#include <fmt/format.h>
#include <boost/container/small_vector.hpp>
#include <boost/regex.hpp>

#ifndef USERVER_NO_RE2_SUPPORT
#include <re2/re2.h>
#endif

#include <userver/compiler/impl/constexpr.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/overloaded.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

class RegexErrorImpl : public RegexError {
public:
    explicit RegexErrorImpl(std::string message) : message_(std::move(message)) {}

    const char* what() const noexcept override { return message_.c_str(); }

private:
    std::string message_;
};

#ifndef USERVER_NO_RE2_SUPPORT

namespace {

constexpr std::size_t kGroupsSboSize = 5;
USERVER_IMPL_CONSTINIT std::atomic<bool> implicit_boost_regex_fallback_allowed{true};

re2::RE2::Options MakeRE2Options() {
    re2::RE2::Options options{};
    options.set_log_errors(false);
    return options;
}

bool ShouldRetryWithBoostRegex(re2::RE2::ErrorCode code) {
    switch (code) {
        case re2::RE2::ErrorCode::ErrorInternal:
        case re2::RE2::ErrorCode::ErrorBadCharClass:
        case re2::RE2::ErrorCode::ErrorRepeatSize:
        case re2::RE2::ErrorCode::ErrorBadPerlOp:
        case re2::RE2::ErrorCode::ErrorPatternTooLarge:
            return true;
        default:
            return false;
    }
}

}  // namespace

struct regex::Impl {
    Impl() = default;

    explicit Impl(std::string_view pattern)
        : regex(std::make_shared<std::variant<re2::RE2, boost::regex>>(
              std::in_place_type<re2::RE2>,
              pattern,
              MakeRE2Options()
          )) {
        {
            const auto& re2_regex = std::get<re2::RE2>(*regex);
            if (re2_regex.ok()) {
                return;
            }
            if (!ShouldRetryWithBoostRegex(re2_regex.error_code())) {
                throw RegexErrorImpl(
                    fmt::format("Failed to construct regex from pattern '{}': {}", pattern, re2_regex.error())
                );
            }
            if (!implicit_boost_regex_fallback_allowed) {
                throw RegexErrorImpl(fmt::format(
                    "Failed to construct regex from pattern '{}': {}. "
                    "Note: boost::regex usage in utils::regex is disallowed for the current service",
                    pattern,
                    re2_regex.error()
                ));
            }
        }
        try {
            regex->emplace<boost::regex>(pattern.begin(), pattern.end());
        } catch (const boost::regex_error& ex) {
            throw RegexErrorImpl(ex.what());
        }
    }

    // Does NOT include the implicit "0th" group that matches the whole pattern.
    std::size_t GetCapturingGroupCount() const {
        UASSERT(regex);
        return utils::Visit(
            *regex,
            [](const re2::RE2& regex) { return static_cast<std::size_t>(regex.NumberOfCapturingGroups()); },
            [](const boost::regex& regex) { return regex.mark_count(); }
        );
    }

    std::shared_ptr<std::variant<re2::RE2, boost::regex>> regex;
};

regex::regex() = default;

regex::regex(std::string_view pattern) : impl_(pattern) {}

regex::regex(const regex&) = default;

regex::regex(regex&& r) noexcept = default;

regex& regex::operator=(const regex&) = default;

regex& regex::operator=(regex&& r) noexcept = default;

regex::~regex() = default;

bool regex::operator==(const regex& other) const { return GetPatternView() == other.GetPatternView(); }

std::string_view regex::GetPatternView() const {
    UASSERT(impl_->regex);
    return utils::Visit(
        *impl_->regex,
        [](const re2::RE2& regex) -> std::string_view { return regex.pattern(); },
        [](const boost::regex& regex) {
            return std::string_view{regex.begin(), regex.size()};
        }
    );
}

std::string regex::str() const { return std::string{GetPatternView()}; }

////////////////////////////////////////////////////////////////

struct match_results::Impl {
    Impl() = default;

    void Prepare(const regex& pattern) {
        UASSERT(pattern.impl_->regex);
        const auto groups_count = pattern.impl_->GetCapturingGroupCount() + 1;
        if (groups_count > groups.size()) {
            groups.resize(groups_count);
        }
    }

    boost::container::small_vector<re2::StringPiece, kGroupsSboSize> groups;
};

match_results::match_results() = default;

match_results::match_results(const match_results&) = default;

match_results& match_results::operator=(const match_results&) = default;

match_results::~match_results() = default;

std::size_t match_results::size() const { return impl_->groups.size(); }

std::string_view match_results::operator[](std::size_t sub) const {
    UASSERT(impl_->groups.size() > sub);
    const auto substr = impl_->groups[sub];
    return {substr.data(), substr.size()};
}

////////////////////////////////////////////////////////////////

bool regex_match(std::string_view str, const regex& pattern) {
    UASSERT(pattern.impl_->regex);
    return utils::Visit(
        *pattern.impl_->regex,
        [&](const re2::RE2& regex) { return re2::RE2::FullMatch(str, regex); },
        [&](const boost::regex& regex) { return boost::regex_match(str.begin(), str.end(), regex); }
    );
}

bool regex_match(std::string_view str, match_results& m, const regex& pattern) {
    UASSERT(pattern.impl_->regex);
    m.impl_->Prepare(pattern);
    return utils::Visit(
        *pattern.impl_->regex,
        [&](const re2::RE2& regex) {
            const bool success = regex.Match(
                str, 0, str.size(), re2::RE2::Anchor::ANCHOR_BOTH, m.impl_->groups.data(), m.impl_->groups.size()
            );
            return success;
        },
        [&](const boost::regex& regex) {
            boost::cmatch boost_matches;
            const bool success = boost::regex_match(str.begin(), str.end(), boost_matches, regex);
            if (!success) {
                return false;
            }
            for (std::size_t i = 0; i < boost_matches.size(); ++i) {
                const auto& sub_match = boost_matches[i];
                m.impl_->groups[i] =
                    std::string_view{&*sub_match.begin(), static_cast<std::size_t>(sub_match.length())};
            }
            return true;
        }
    );
}

bool regex_search(std::string_view str, const regex& pattern) {
    UASSERT(pattern.impl_->regex);
    return utils::Visit(
        *pattern.impl_->regex,
        [&](const re2::RE2& regex) { return re2::RE2::PartialMatch(str, regex); },
        [&](const boost::regex& regex) { return boost::regex_search(str.begin(), str.end(), regex); }
    );
}

bool regex_search(std::string_view str, match_results& m, const regex& pattern) {
    UASSERT(pattern.impl_->regex);
    m.impl_->Prepare(pattern);
    return utils::Visit(
        *pattern.impl_->regex,
        [&](const re2::RE2& regex) {
            const bool success = regex.Match(
                str, 0, str.size(), re2::RE2::Anchor::UNANCHORED, m.impl_->groups.data(), m.impl_->groups.size()
            );
            return success;
        },
        [&](const boost::regex& regex) {
            boost::cmatch boost_matches;
            const bool success = boost::regex_search(str.begin(), str.end(), boost_matches, regex);
            if (!success) {
                return false;
            }
            for (std::size_t i = 0; i < boost_matches.size(); ++i) {
                const auto& sub_match = boost_matches[i];
                m.impl_->groups[i] =
                    std::string_view{&*sub_match.begin(), static_cast<std::size_t>(sub_match.length())};
            }
            return true;
        }
    );
}

std::string regex_replace(std::string_view str, const regex& pattern, std::string_view repl) {
    UASSERT(pattern.impl_->regex);
    std::string res;
    res.reserve(str.size() + str.size() / 4);

    utils::Visit(
        *pattern.impl_->regex,
        [&](const re2::RE2& regex) {
            re2::StringPiece match;

            while (true) {
                const bool success = regex.Match(str, 0, str.size(), re2::RE2::Anchor::UNANCHORED, &match, 1);
                if (!success) {
                    res += str;
                    break;
                }

                const auto non_matched_part = str.substr(0, match.data() - str.data());
                res += non_matched_part;
                res += repl;

                str.remove_prefix(non_matched_part.size() + match.size());
                if (__builtin_expect(match.size() == 0, false)) {
                    if (str.empty()) {
                        break;
                    }
                    // Prevent infinite loop on matching an empty substring.
                    res += str[0];
                    str.remove_prefix(1);
                }
            }
        },
        [&](const boost::regex& regex) {
            boost::regex_replace(
                std::back_inserter(res), str.begin(), str.end(), regex, repl, boost::regex_constants::format_literal
            );
        }
    );

    return res;
}

std::string regex_replace(std::string_view str, const regex& pattern, Re2Replacement repl) {
    UASSERT(pattern.impl_->regex);
    std::string res;
    res.reserve(str.size() + str.size() / 4);

    utils::Visit(
        *pattern.impl_->regex,
        [&](const re2::RE2& regex) {
            res.assign(str);
            re2::RE2::GlobalReplace(&res, regex, repl.replacement);
        },
        [&](const boost::regex& /*regex*/) {
            // Unsupported, because repl is re2-specific.
            throw RegexErrorImpl(
                fmt::format("regex_replace is unsupported with boost::regex '{}'", pattern.GetPatternView())
            );
        }
    );

    return res;
}

bool IsImplicitBoostRegexFallbackAllowed() noexcept { return implicit_boost_regex_fallback_allowed.load(); }

void SetImplicitBoostRegexFallbackAllowed(bool allowed) noexcept { implicit_boost_regex_fallback_allowed = allowed; }

#else

struct regex::Impl {
    boost::regex r;

    Impl() = default;

    explicit Impl(std::string_view pattern) try : r(pattern.begin(), pattern.end()) {
    } catch (const boost::regex_error& ex) {
        throw RegexErrorImpl(ex.what());
    }
};

regex::regex() = default;

regex::regex(std::string_view pattern) : impl_(regex::Impl(pattern)) {}

regex::~regex() = default;

regex::regex(const regex&) = default;

regex::regex(regex&& r) noexcept { impl_->r.swap(r.impl_->r); }

regex& regex::operator=(const regex&) = default;

regex& regex::operator=(regex&& r) noexcept {
    impl_->r.swap(r.impl_->r);
    return *this;
}

bool regex::operator==(const regex& other) const { return impl_->r == other.impl_->r; }

std::string_view regex::GetPatternView() const { return std::string_view{impl_->r.expression(), impl_->r.size()}; }

std::string regex::str() const { return std::string{GetPatternView()}; }

////////////////////////////////////////////////////////////////

struct match_results::Impl {
    boost::cmatch m;

    Impl() = default;
};

match_results::match_results() = default;

match_results::~match_results() = default;

match_results::match_results(const match_results&) = default;

match_results& match_results::operator=(const match_results&) = default;

std::size_t match_results::size() const { return impl_->m.size(); }

std::string_view match_results::operator[](std::size_t sub) const {
    auto substr = impl_->m[sub];
    return {&*substr.begin(), static_cast<std::size_t>(substr.length())};
}

////////////////////////////////////////////////////////////////

bool regex_match(std::string_view str, const regex& pattern) {
    return boost::regex_match(str.begin(), str.end(), pattern.impl_->r);
}

bool regex_match(std::string_view str, match_results& m, const regex& pattern) {
    return boost::regex_match(str.begin(), str.end(), m.impl_->m, pattern.impl_->r);
}

bool regex_search(std::string_view str, const regex& pattern) {
    return boost::regex_search(str.begin(), str.end(), pattern.impl_->r);
}

bool regex_search(std::string_view str, match_results& m, const regex& pattern) {
    return boost::regex_search(str.begin(), str.end(), m.impl_->m, pattern.impl_->r);
}

std::string regex_replace(std::string_view str, const regex& pattern, std::string_view repl) {
    std::string res;
    res.reserve(str.size() + str.size() / 4);

    boost::regex_replace(
        std::back_inserter(res), str.begin(), str.end(), pattern.impl_->r, repl, boost::regex_constants::format_literal
    );

    return res;
}

bool IsImplicitBoostRegexFallbackAllowed() noexcept { return true; }

void SetImplicitBoostRegexFallbackAllowed(bool allowed) noexcept {
    UINVARIANT(allowed, "Cannot stop using boost::regex, because re2 is disabled");
}

#endif

}  // namespace utils

USERVER_NAMESPACE_END
