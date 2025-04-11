#include <userver/utils/regex.hpp>

#include <iterator>
#include <memory>
#include <optional>
#include <string>

#include <fmt/format.h>
#include <re2/re2.h>
#include <boost/container/small_vector.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

class RegexErrorImpl : public RegexError {
public:
    explicit RegexErrorImpl(std::string message) : message_(std::move(message)) {}

    const char* what() const noexcept override { return message_.c_str(); }

private:
    std::string message_;
};

namespace {

constexpr std::size_t kGroupsSboSize = 5;

re2::RE2::Options MakeRE2Options() {
    re2::RE2::Options options{};
    options.set_log_errors(false);
    return options;
}

}  // namespace

class regex::Impl {
public:
    Impl() = default;

    explicit Impl(std::string_view pattern) : regex_(std::make_shared<const re2::RE2>(pattern, MakeRE2Options())) {
        if (regex_->ok()) {
            return;
        }
        throw RegexErrorImpl(fmt::format("Failed to construct regex from pattern '{}': {}", pattern, regex_->error()));
    }

    const re2::RE2& Get() const {
        UASSERT(regex_);
        return *regex_;
    }

    // Does NOT include the implicit "0th" group that matches the whole pattern.
    std::size_t GetCapturingGroupCount() const { return static_cast<std::size_t>(Get().NumberOfCapturingGroups()); }

private:
    std::shared_ptr<const re2::RE2> regex_;
};

regex::regex() = default;

regex::regex(std::string_view pattern) : impl_(pattern) {}

regex::regex(const regex&) = default;

regex::regex(regex&& r) noexcept = default;

regex& regex::operator=(const regex&) = default;

regex& regex::operator=(regex&& r) noexcept = default;

regex::~regex() = default;

bool regex::operator==(const regex& other) const { return GetPatternView() == other.GetPatternView(); }

std::string_view regex::GetPatternView() const { return impl_->Get().pattern(); }

std::string regex::str() const { return std::string{GetPatternView()}; }

////////////////////////////////////////////////////////////////

struct match_results::Impl {
    Impl() = default;

    void Prepare(std::string_view target, const regex& pattern) {
        this->target = target;

        const auto groups_count = pattern.impl_->GetCapturingGroupCount() + 1;
        if (groups_count > groups.size()) {
            groups.resize(groups_count);
        }
    }

    std::string_view target;
    boost::container::small_vector<re2::StringPiece, kGroupsSboSize> groups;
};

match_results::match_results() = default;

match_results::match_results(const match_results&) = default;

match_results& match_results::operator=(const match_results&) = default;

match_results::~match_results() = default;

std::size_t match_results::size() const { return impl_->groups.size(); }

std::string_view match_results::operator[](std::size_t sub) const {
    UASSERT(sub < size());
    const auto substr = impl_->groups[sub];
    return {substr.data(), substr.size()};
}

std::size_t match_results::position(std::size_t sub) const {
    UASSERT(sub < size());
    const auto substr = impl_->groups[sub];
    UINVARIANT(
        sub == 0 || !substr.empty(),
        fmt::format(
            "Trying to access position of capturing group {}, which is empty (missing), target='{}'", sub, impl_->target
        )
    );
    return substr.data() - impl_->target.data();
}

std::size_t match_results::length(std::size_t sub) const {
    UASSERT(sub < size());
    return impl_->groups[sub].size();
}

std::string_view match_results::prefix() const {
    UASSERT_MSG(size() > 0, "Empty match_results object");
    return impl_->target.substr(0, position(0));
}

std::string_view match_results::suffix() const {
    UASSERT_MSG(size() > 0, "Empty match_results object");
    return impl_->target.substr(position(0) + impl_->groups[0].size());
}

////////////////////////////////////////////////////////////////

bool regex_match(std::string_view str, const regex& pattern) { return re2::RE2::FullMatch(str, pattern.impl_->Get()); }

bool regex_match(std::string_view str, match_results& m, const regex& pattern) {
    m.impl_->Prepare(str, pattern);
    const bool success = pattern.impl_->Get().Match(
        str, 0, str.size(), re2::RE2::Anchor::ANCHOR_BOTH, m.impl_->groups.data(), m.impl_->groups.size()
    );
    return success;
}

bool regex_search(std::string_view str, const regex& pattern) {
    return re2::RE2::PartialMatch(str, pattern.impl_->Get());
}

bool regex_search(std::string_view str, match_results& m, const regex& pattern) {
    m.impl_->Prepare(str, pattern);
    const bool success = pattern.impl_->Get().Match(
        str, 0, str.size(), re2::RE2::Anchor::UNANCHORED, m.impl_->groups.data(), m.impl_->groups.size()
    );
    return success;
}

std::string regex_replace(std::string_view str, const regex& pattern, std::string_view repl) {
    std::string res;
    res.reserve(str.size() + str.size() / 4);

    const auto& regex = pattern.impl_->Get();
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

    return res;
}

std::string regex_replace(std::string_view str, const regex& pattern, Re2Replacement repl) {
    std::string res;
    res.reserve(str.size() + str.size() / 4);

    res.assign(str);
    re2::RE2::GlobalReplace(&res, pattern.impl_->Get(), repl.replacement);

    return res;
}

}  // namespace utils

USERVER_NAMESPACE_END
