#include <userver/formats/json/validate.hpp>

#include <rapidjson/reader.h>
#include <rapidjson/schema.h>

#include <formats/json/impl/accept.hpp>
#include <userver/formats/json/impl/types.hpp>
#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json {

namespace impl {

using SchemaDocument =
  rapidjson::GenericSchemaDocument<impl::Value, rapidjson::CrtAllocator>;

using SchemaValidator = rapidjson::GenericSchemaValidator<
  impl::SchemaDocument, rapidjson::BaseReaderHandler<impl::UTF8, void>,
  rapidjson::CrtAllocator>;

} // namespace impl

struct Schema::Impl final {
  impl::SchemaDocument schemaDocument;
};

Schema::Schema(const Value& doc)
  : pimpl_(Impl{impl::SchemaDocument{doc.GetNative()}}) {}

Schema::~Schema() = default;

struct SchemaValidator::Impl final : public impl::SchemaValidator {
  using impl::SchemaValidator::GenericSchemaValidator;
};

SchemaValidator::SchemaValidator(const Schema& schema)
  : pimpl_(schema.pimpl_->schemaDocument) {}

SchemaValidator::~SchemaValidator() = default;

bool SchemaValidator::Validate(const Value& doc) {
  if (!pimpl_->IsValid())
    pimpl_->ResetError();
  return AcceptNoRecursion(doc.GetNative(), *pimpl_);
}

Value SchemaValidator::GetError() const {
  return pimpl_->GetError();
}


bool Validate(const Value& doc, const Schema& schema) {
  impl::SchemaValidator validator(schema.pimpl_->schemaDocument);
  return AcceptNoRecursion(doc.GetNative(), validator);
}

} // namespace formats::json

USERVER_NAMESPACE_END
