#include <userver/chaotic/sax_parser.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::sax::impl {

void UnknownSubscriber::Reset() { builder_ = {formats::json::Type::kObject}; }

void UnknownSubscriber::SetKey(std::string_view key) { key_ = key; }

void UnknownSubscriber::OnSend(formats::json::Value&& value) {
    builder_.EmplaceNocheck(std::move(key_), std::move(value));
}

formats::json::Value UnknownSubscriber::ExtractValue() { return builder_.ExtractValue(); }

}  // namespace chaotic::sax::impl

USERVER_NAMESPACE_END
