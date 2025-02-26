#pragma once

/// @file userver/utils/regex.hpp
/// @brief @copybrief utils::regex

#include <cstddef>
#include <exception>
#include <string_view>

#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

class match_results;
struct Re2Replacement;

/// Thrown from constructors of @ref utils::regex with an invalid pattern.
class RegexError : public std::exception {};

/// @ingroup userver_universal userver_containers
///
/// @brief A drop-in replacement for `std::regex` without huge includes
/// and with better performance characteristics.
///
/// utils::regex is currently implemented using re2.
///
/// @see @ref utils::regex_match
/// @see @ref utils::regex_search
/// @see @ref utils::regex_replace
///
/// Read [re2 documentation](https://github.com/google/re2/wiki/syntax) on the limitations of re2 engine.
/// Notably, it does not support:
///
/// 1. lookahead and lookbehind;
/// 2. quantifiers over 1000, regexes with large repetition counts consume more memory;
/// 3. spaces in quantifiers like `\w{1, 5}`;
/// 4. possessive quantifiers.
///
/// ## An example of complex string parsing using `utils::regex`
///
/// @snippet utils/regex_test.cpp  split text
class regex final {
public:
    /// Constructs a null regex, any usage except for copy/move is UB.
    regex();

    /// @brief Compiles regex from pattern, always valid on construction.
    /// @throws utils::InvalidRegex if @a pattern is invalid
    explicit regex(std::string_view pattern);

    regex(const regex&);
    regex(regex&&) noexcept;
    regex& operator=(const regex&);
    regex& operator=(regex&&) noexcept;
    ~regex();

    /// @returns `true` if the patterns are equal.
    /// @note May also return `true` if the patterns are not equal, but are equivalent.
    bool operator==(const regex&) const;

    /// @returns a view to the original pattern stored inside.
    std::string_view GetPatternView() const;

    /// @returns the original pattern.
    std::string str() const;

private:
    class Impl;
    utils::FastPimpl<Impl, 16, 8> impl_;

    friend class match_results;
    friend bool regex_match(std::string_view str, const regex& pattern);
    friend bool regex_match(std::string_view str, match_results& m, const regex& pattern);
    friend bool regex_search(std::string_view str, const regex& pattern);
    friend bool regex_search(std::string_view str, match_results& m, const regex& pattern);
    friend std::string regex_replace(std::string_view str, const regex& pattern, std::string_view repl);
    friend std::string regex_replace(std::string_view str, const regex& pattern, Re2Replacement repl);
};

/// @ingroup userver_universal userver_containers
///
/// @brief A drop-in replacement for `std::match_results` without huge includes
/// and with better performance characteristics. Represents capturing groups of a single match result.
///
/// The group 0 always matches the whole pattern. User groups start with index 1.
///
/// Non-empty groups always point within the source string, so the position of a group within the source string
/// can be obtained by subtracting `.data()` pointers or `.begin()` iterators.
///
/// @warning The implementation can return empty groups as `std::string_view`s with `data() == nullptr` or some invalid
/// pointer with `size() == 0`. Check for emptiness before performing pointer arithmetic if a group can be empty
/// according to the regex!
///
/// @see utils::regex
class match_results final {
public:
    /// Constructs a null `match_results`, any usage except for copy/move is UB.
    /// Filled upon successful @ref regex_match or @ref regex_search.
    match_results();

    match_results(const match_results&);
    match_results& operator=(const match_results&);
    ~match_results();

    /// @returns the number of capturing groups, including the group 0.
    std::size_t size() const;

    /// @returns the capturing group at @a sub.
    /// @note Group 0 always matches the whole pattern. User groups start with index 1.
    std::string_view operator[](std::size_t sub) const;

    /// @returns the position of the first character of the capturing group @a sub within the target (haystack) string.
    /// @note Group 0 always matches the whole pattern. User groups start with index 1.
    /// @warning For empty groups, calling this method is UB. Group 0 is always valid.
    std::size_t position(std::size_t sub) const;

    /// @returns the length of the capturing group at @a sub.
    /// @note Group 0 always matches the whole pattern. User groups start with index 1.
    std::size_t length(std::size_t sub) const;

    /// @returns the substring from the beginning of the target (haystack) string to the beginning of the full match.
    std::string_view prefix() const;

    /// @returns the substring from the end of the full match to the end of the target (haystack) string.
    std::string_view suffix() const;

private:
    struct Impl;
    utils::FastPimpl<Impl, 120, 8> impl_;

    friend bool regex_match(std::string_view str, const regex& pattern);
    friend bool regex_match(std::string_view str, match_results& m, const regex& pattern);
    friend bool regex_search(std::string_view str, const regex& pattern);
    friend bool regex_search(std::string_view str, match_results& m, const regex& pattern);
    friend std::string regex_replace(std::string_view str, const regex& pattern, std::string_view repl);
    friend std::string regex_replace(std::string_view str, const regex& pattern, Re2Replacement repl);
};

/// @brief Determines whether the regular expression matches the entire target
/// character sequence
bool regex_match(std::string_view str, const regex& pattern);

/// @brief Returns true if the specified regular expression matches
/// the whole of the input. Fills in what matched in m.
/// @note @a m may be clobbered on failure.
bool regex_match(std::string_view str, match_results& m, const regex& pattern);

/// @brief Determines whether the regular expression matches anywhere in the
/// target character sequence
bool regex_search(std::string_view str, const regex& pattern);

/// @brief Determines whether the regular expression matches anywhere in the
/// target character sequence. Fills in what matched in m.
/// @note @a m may be clobbered on failure.
bool regex_search(std::string_view str, match_results& m, const regex& pattern);

/// @brief Create a new string where all regular expression matches replaced
/// with repl.
///
/// Interprets @a repl as a literal, does not support substitutions.
///
/// @see utils::Re2Replacement
std::string regex_replace(std::string_view str, const regex& pattern, std::string_view repl);

/// @brief Replacement string with substitution support
///
/// @warning Avoid if at all possible, prefer using vanilla
/// @ref utils::regex_replace, as it is more portable
///
/// @warning Allowing user-provided strings in @a replacement leads
/// to injection vulnerabilities!
///
/// May contain the following special syntax:
///
/// * `\N` (spelled as `\\N` in C++ string literals), where 0 <= N <= 9,
///   can be used to insert capture groups;
/// * In particular, `\0` refers to the contents of the whole match;
/// * Literal `\` should be escaped as `\\`
///   (spelled as `\\\\` in C++ string literals)
///
/// @see utils::regex_replace
struct Re2Replacement final {
    std::string_view replacement;
};

/// @overload
/// @see utils::Re2Replacement
std::string regex_replace(std::string_view str, const regex& pattern, Re2Replacement repl);

}  // namespace utils

USERVER_NAMESPACE_END
