#pragma once

/// @file userver/storages/query.hpp
/// @brief @copybrief storages::Query

#include <optional>
#include <string>
#include <variant>

#include <userver/utils/string_literal.hpp>
#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {
class Span;
}  // namespace tracing

namespace storages {

/// @brief Holds a query, its name and logging mode; used by all the SQL databases of userver.
///
/// Prefer using via @ref scripts/docs/en/userver/sql_files.md or use a const variable with `name` specified
/// as NameLiteral.
///
/// @note You may write a query in `.sql` file and generate a header file with Query from it.
///       See @ref scripts/docs/en/userver/sql_files.md for more information.
class Query {
public:
    /// String with query name
    using Name = utils::StrongTypedef<struct NameTag, std::string>;

    /// Zero terminated view to the query name
    using NameView = utils::zstring_view;

    /// Compile time literal with query name
    class NameLiteral : public utils::zstring_view {
    public:
        NameLiteral() = delete;

#if defined(__clang__) && __clang_major__ < 18
        // clang-16 and below lose (optimize out) the pointer to `literal` with consteval. Clang-18 is know to work
        constexpr
#else
        USERVER_IMPL_CONSTEVAL
#endif
            NameLiteral(const char* literal) noexcept
            : zstring_view{literal} {
        }
    };

    enum class LogMode : unsigned char {
        kFull,      ///< Output name and optionally statement
        kNameOnly,  ///< Output only name
    };

    Query() = default;
    ~Query() = default;

    Query(const Query& other) = default;
    Query(Query&& other) = default;
    Query& operator=(const Query& other) = default;
    Query& operator=(Query&& other) = default;

    /// Constructor that omits dynamic initialization and relies on `statement` and `name` being string literals
    constexpr Query(utils::StringLiteral statement, NameLiteral name, LogMode log_mode)
        : data_{StaticStrings{statement, name}}, log_mode_{log_mode} {}

    Query(const char* statement, std::optional<Name> name = std::nullopt, LogMode log_mode = LogMode::kFull)
        : Query(std::string{statement}, std::move(name), log_mode) {}
    Query(std::string statement, std::optional<Name> name = std::nullopt, LogMode log_mode = LogMode::kFull)
        : data_{DynamicStrings{std::move(statement), std::move(name)}}, log_mode_(log_mode) {}

    /// @returns view to the query name that is alive as long as *this is alive
    std::optional<NameView> GetOptionalNameView() const noexcept;

    /// @returns optional queury name suitable for construction of a new Query
    std::optional<Name> GetOptionalName() const { return std::optional<Name>{GetOptionalNameView()}; }

    /// @returns zero terminated view to the statement that is alive as long as *this is alive
    utils::zstring_view GetStatementView() const noexcept;

    /// @returns logging mode
    LogMode GetLogMode() const noexcept { return log_mode_; }

private:
    struct DynamicStrings {
        std::string statement;
        std::optional<Name> name;
    };
    struct StaticStrings {
        utils::StringLiteral statement;
        std::optional<NameLiteral> name;
    };
    struct NameViewVisitor;

    std::variant<StaticStrings, DynamicStrings> data_ = StaticStrings{utils::StringLiteral{""}, std::nullopt};
    LogMode log_mode_ = LogMode::kFull;
};

}  // namespace storages

USERVER_NAMESPACE_END
