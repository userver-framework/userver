#include <userver/formats/json/validate.hpp>

#include <algorithm>
#include <fstream>
#include <string_view>

#include <fmt/format.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/istreamwrapper.h>

#include <rapidjson/schema.h>

#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/impl/types.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json {

namespace impl {

using SchemaDocument =
    rapidjson::GenericSchemaDocument<impl::Value, rapidjson::CrtAllocator>;

using SchemaValidator = rapidjson::GenericSchemaValidator<
    impl::SchemaDocument, rapidjson::BaseReaderHandler<impl::UTF8, void>,
    rapidjson::CrtAllocator>;

}  // namespace impl

namespace {

::rapidjson::CrtAllocator g_allocator;

impl::Document DocumentFromString(std::string_view doc) {
  if (doc.empty()) {
    throw ParseException("JSON document is empty");
  }

  impl::Document json{&g_allocator};
  rapidjson::ParseResult ok =
      json.Parse<rapidjson::kParseDefaultFlags |
                 rapidjson::kParseIterativeFlag |
                 rapidjson::kParseFullPrecisionFlag>(doc.data(), doc.size());
  if (!ok) {
    const auto offset = ok.Offset();
    const auto line = 1 + std::count(doc.begin(), doc.begin() + offset, '\n');
    const auto from_pos = doc.substr(0, offset).find_last_of('\n');
    const auto column = offset > from_pos ? offset - from_pos : offset + 1;

    throw ParseException(
        fmt::format("JSON parse error at line {} column {}: {}", line, column,
                    rapidjson::GetParseError_En(ok.Code())));
  }

  return json;
}

impl::Document DocumentFromStream(std::istream& is) {
  if (!is) {
    throw BadStreamException(is);
  }

  rapidjson::IStreamWrapper in(is);
  impl::Document json{&g_allocator};
  rapidjson::ParseResult ok =
      json.ParseStream<rapidjson::kParseDefaultFlags |
                       rapidjson::kParseIterativeFlag |
                       rapidjson::kParseFullPrecisionFlag>(in);
  if (!ok) {
    throw ParseException(fmt::format("JSON parse error at offset {}: {}",
                                     ok.Offset(),
                                     rapidjson::GetParseError_En(ok.Code())));
  }

  return json;
}

}  // namespace

struct Schema::Impl final {
  impl::SchemaDocument schema;

  bool Accept(impl::Document doc) const {
    impl::SchemaValidator validator{schema};
    return doc.Accept(validator);
  }
};

Schema::Schema(std::string_view doc)
    : pimpl_(Impl{impl::SchemaDocument{DocumentFromString(doc)}}) {}

Schema::Schema(std::istream& is)
    : pimpl_(Impl{impl::SchemaDocument{DocumentFromStream(is)}}) {}

Schema::~Schema() = default;

bool Validate(std::string_view doc, const formats::json::Schema& schema) {
  return schema.pimpl_->Accept(DocumentFromString(doc));
}

bool Validate(std::istream& is, const formats::json::Schema& schema) {
  return schema.pimpl_->Accept(DocumentFromStream(is));
}

}  // namespace formats::json

USERVER_NAMESPACE_END
