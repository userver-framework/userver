#include <userver/formats/json/schema.hpp>

#include <rapidjson/reader.h>
#include <rapidjson/schema.h>

#include <formats/json/impl/accept.hpp>
#include <userver/formats/json/impl/types.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json {

namespace {

std::string ToStdString(const impl::Value& value) {
    UASSERT(value.IsString());
    return std::string{value.GetString(), value.GetStringLength()};
}

}  // namespace

namespace impl {

using SchemaDocument = rapidjson::GenericSchemaDocument<impl::Value, rapidjson::CrtAllocator>;

using SchemaValidator = rapidjson::GenericSchemaValidator<
    impl::SchemaDocument,
    rapidjson::BaseReaderHandler<impl::UTF8, void>,
    rapidjson::CrtAllocator>;

}  // namespace impl

SchemaValidationException::SchemaValidationException(
    std::string_view msg,
    std::string_view path,
    std::string_view schema_path
)
    : ExceptionWithPath(msg, path), schema_path_(schema_path) {}

std::string_view SchemaValidationException::GetSchemaPath() const noexcept { return schema_path_; }

struct Schema::ValidationResult::Impl final {
    impl::Value error;
};

Schema::ValidationResult::ValidationResult() noexcept = default;

Schema::ValidationResult::ValidationResult(ValidationResult&&) noexcept = default;

Schema::ValidationResult& Schema::ValidationResult::operator=(ValidationResult&&) noexcept = default;

Schema::ValidationResult::~ValidationResult() noexcept = default;

Schema::ValidationResult::operator bool() const noexcept { return IsValid(); }

bool Schema::ValidationResult::IsValid() const noexcept {
    UASSERT(impl_->error.IsObject());
    return impl_->error.ObjectEmpty();
}

bool Schema::ValidationResult::IsError() const noexcept { return !IsValid(); }

void Schema::ValidationResult::ThrowIfError() && {
    if (IsError()) {
        std::move(*this).GetError().Throw();
    }
}

Schema::ValidationError Schema::ValidationResult::GetError() && {
    UINVARIANT(
        IsError(),
        "Attempted to access error details while there is no error "
        "(validation succeeded)"
    );
    auto& error = impl_->error;

    ValidationError result;

    auto iter = error.FindMember("type");
    if (iter == error.MemberEnd()) {
        iter = error.MemberBegin();
    }
    auto& details = iter->value;

    details.EraseMember("errorCode");

    const auto instance_ref = details.FindMember("instanceRef");
    result.value_path_ = ToStdString(instance_ref->value);
    details.EraseMember(instance_ref);

    const auto schema_ref = details.FindMember("schemaRef");
    result.schema_path_ = ToStdString(schema_ref->value);
    details.EraseMember(schema_ref);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer writer(buffer);
    details.Accept(writer);
    result.details_string_ = std::string{buffer.GetString(), buffer.GetLength()};

    return result;
}

Schema::ValidationError::ValidationError() = default;

void Schema::ValidationError::Throw() const {
    // ExceptionWithPath expects path in the format of formats::common::Path.
    // For normal paths, it's "foo/bar"; for root, it's "/".
    // RapidJSON provides path in "#/foo/bar" format. So we need to cut up to 2
    // characters from the beginning.
    const auto cut_path = value_path_.size() <= 2 ? std::string_view{"/"} : std::string_view{value_path_}.substr(2);

    throw SchemaValidationException(details_string_, cut_path, schema_path_);
}

std::string_view Schema::ValidationError::GetValuePath() const { return value_path_; }

std::string_view Schema::ValidationError::GetSchemaPath() const { return schema_path_; }

std::string_view Schema::ValidationError::GetDetailsString() const { return details_string_; }

struct Schema::Impl final {
    impl::SchemaDocument schema_document;
};

Schema::Schema(const Value& doc) : impl_(Impl{impl::SchemaDocument{doc.GetNative()}}) {}

Schema::~Schema() noexcept = default;

Schema::ValidationResult Schema::Validate(const Value& doc) const {
    impl::SchemaValidator validator(impl_->schema_document);
    AcceptNoRecursion(doc.GetNative(), validator);

    ValidationResult result;
    result.impl_->error = std::move(validator.GetError());
    return result;
}

}  // namespace formats::json

USERVER_NAMESPACE_END
