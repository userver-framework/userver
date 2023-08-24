#include <userver/formats/bson/inline.hpp>

#include <formats/bson/value_impl.hpp>
#include <formats/bson/wrappers.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::bson::impl {

class InlineArrayBuilder::Helper : public ArrayIndexer {};

Document InlineDocBuilder::Build() { return Document(builder_.Extract()); }

InlineArrayBuilder::InlineArrayBuilder() = default;
InlineArrayBuilder::~InlineArrayBuilder() = default;

Value InlineArrayBuilder::Build() {
  return Value(std::make_shared<ValueImpl>(builder_.Extract(),
                                           ValueImpl::DocumentKind::kArray));
}

std::string_view InlineArrayBuilder::GetKey() {
  auto key = helper_->GetKey();
  helper_->Advance();
  return key;
}

}  // namespace formats::bson::impl

USERVER_NAMESPACE_END
