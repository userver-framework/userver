#pragma once

/// @file userver/formats/json/schema.hpp
/// @brief JSON schema validator

#include <string>
#include <string_view>

#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json {

/// @see formats::json::Schema::ValidationError::Throw
class SchemaValidationException final : public ExceptionWithPath {
public:
    SchemaValidationException(std::string_view msg, std::string_view path, std::string_view schema_path);

    std::string_view GetSchemaPath() const noexcept;

private:
    std::string schema_path_;
};

/// @brief Contains a prepared JSON schema.
///
/// Usage example:
/// @snippet formats/json/schema_test.cpp  sample
class Schema final {
public:
    class ValidationResult;
    class ValidationError;

    explicit Schema(const Value& doc);
    ~Schema() noexcept;

    /// @brief Validates JSON against the Schema.
    /// @returns ValidationResult describing the validation error, if any.
    ValidationResult Validate(const Value& doc) const;

private:
    struct Impl;
    static constexpr std::size_t kSize = 288;
    static constexpr std::size_t kAlignment = 8;
    utils::FastPimpl<Impl, kSize, kAlignment> impl_;
};

/// Contains error details (if any) from Schema::Validate.
class Schema::ValidationResult final {
public:
    /// Creates an `IsValid` result.
    ValidationResult() noexcept;

    ValidationResult(ValidationResult&&) noexcept;
    ValidationResult& operator=(ValidationResult&&) noexcept;
    ~ValidationResult() noexcept;

    /// @returns `true` on validation success.
    explicit operator bool() const noexcept;

    /// @returns `true` on validation success.
    bool IsValid() const noexcept;

    /// @returns `true` on validation error.
    bool IsError() const noexcept;

    /// @throws SchemaValidationException on validation error.
    /// @see Schema::ValidationResult::GetError
    /// @see Schema::ValidationError::Throw
    void ThrowIfError() &&;

    /// @returns Validation error, `IsError` must be true.
    ValidationError GetError() &&;

private:
    friend class Schema;
    friend class ValidationError;

    struct Impl;
    static constexpr std::size_t kSize = 24;
    static constexpr std::size_t kAlignment = 8;
    utils::FastPimpl<Impl, kSize, kAlignment> impl_;
};

class Schema::ValidationError final {
public:
    /// @throws SchemaValidationException with error details.
    /// @see Schema::ValidationResult::ThrowIfError
    [[noreturn]] void Throw() const;

    /// Describes the location within the validated JSON which violates
    /// schema. The exact format of value path is unstable and should not
    /// be relied upon.
    std::string_view GetValuePath() const;
    /// Describes the location within the schema which was violated. The exact
    /// format of schema path is unstable and should not be relied upon.
    std::string_view GetSchemaPath() const;
    /// Describes the specifics of what condition was violated. The exact
    /// format of details is unstable and should not be relied upon.
    std::string_view GetDetailsString() const;

private:
    friend class ValidationResult;

    ValidationError();

    std::string value_path_;
    std::string schema_path_;
    std::string details_string_;
};

}  // namespace formats::json

USERVER_NAMESPACE_END
