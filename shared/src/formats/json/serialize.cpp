#include <userver/formats/json/serialize.hpp>

#include <algorithm>
#include <array>
#include <fstream>
#include <memory>
#include <string_view>
#include <vector>

#include <fmt/format.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <boost/container/small_vector.hpp>

#include <formats/json/impl/json_tree.hpp>
#include <formats/json/impl/types_impl.hpp>
#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json {

namespace {

::rapidjson::CrtAllocator g_allocator;

std::string_view AsStringView(const impl::Value& jval) {
  return {jval.GetString(), jval.GetStringLength()};
}

void CheckKeyUniqueness(const impl::Value* root) {
  using KeysStack = boost::container::small_vector<std::string_view,
                                                   impl::kInitialStackDepth>;

  impl::TreeStack stack;
  const impl::Value* value = root;

  stack.emplace_back();  // fake "top" frame to avoid extra checks for an empty
                         // stack inside walker loop
  KeysStack keys;
  for (;;) {
    stack.back().Advance();
    if (value->IsObject()) {
      const std::size_t count = value->MemberCount();
      const auto begin = value->MemberBegin();
      if (count > keys.size()) {
        keys.resize(count);
      }
      for (std::size_t i = 0; i < count; ++i) {
        keys[i] = AsStringView(begin[i].name);
      }
      std::sort(keys.begin(), keys.begin() + count);
      const auto* cons_eq_element =
          std::adjacent_find(keys.data(), keys.data() + count);
      if (cons_eq_element != keys.data() + count) {
        throw ParseException("Duplicate key: " + std::string(*cons_eq_element) +
                             " at " + impl::ExtractPath(stack));
      }
    }

    if ((value->IsObject() && value->MemberCount() > 0) ||
        (value->IsArray() && value->Size() > 0)) {
      // descend
      stack.emplace_back(value);
    } else {
      while (!stack.back().HasMoreElements()) {
        stack.pop_back();
        if (stack.empty()) return;
      }
    }

    value = stack.back().CurrentValue();
  }
}

impl::VersionedValuePtr EnsureValid(impl::Document&& json) {
  CheckKeyUniqueness(&json);

  return impl::VersionedValuePtr::Create(std::move(json));
}

// Like `GenericValue.Accept`, but the order of the keys in objects is sorted
template <typename Handler>
bool AcceptStable(const impl::Value& origin, Handler& handler) {
  switch (origin.GetType()) {
    case rapidjson::kArrayType: {
      if (!handler.StartArray()) return false;
      for (const auto* v = origin.Begin(); v != origin.End(); v++) {
        if (!AcceptStable(*v, handler)) return false;
      }
      return handler.EndArray(origin.Size());
    }
    case rapidjson::kObjectType: {
      std::vector<std::pair<std::string, impl::Value::ConstMemberIterator>>
          keys;
      keys.reserve(origin.MemberCount());
      if (!handler.StartObject()) return false;
      for (auto m = origin.MemberBegin(); m != origin.MemberEnd(); ++m) {
        UASSERT(m->name.IsString());
        keys.emplace_back(m->name.GetString(), m);
      }
      std::sort(keys.begin(), keys.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.first < rhs.first;
      });
      for (const auto& elem : keys) {
        if (!handler.Key(elem.second->name.GetString(),
                         elem.second->name.GetStringLength()))
          return false;
        if (!AcceptStable(elem.second->value, handler)) return false;
      }
      return handler.EndObject(origin.MemberCount());
    }
    default: {
      return origin.Accept(handler);
    }
  }
}

}  // namespace

Value FromString(std::string_view doc) {
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
    // Some versions of libstdc++ have runtime isues in
    // string_view::find_last_of("\n", 0, offset) implementation.
    const auto from_pos = doc.substr(0, offset).find_last_of('\n');
    const auto column = offset > from_pos ? offset - from_pos : offset + 1;

    throw ParseException(
        fmt::format("JSON parse error at line {} column {}: {}", line, column,
                    rapidjson::GetParseError_En(ok.Code())));
  }

  return Value{EnsureValid(std::move(json))};
}

Value FromStream(std::istream& is) {
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

  return Value{EnsureValid(std::move(json))};
}

void Serialize(const Value& doc, std::ostream& os) {
  rapidjson::OStreamWrapper out{os};
  rapidjson::Writer writer(out);
  doc.GetNative().Accept(writer);
  if (!os) {
    throw BadStreamException(os);
  }
}

std::string ToString(const Value& doc) {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer writer(buffer);
  doc.GetNative().Accept(writer);
  return std::string{buffer.GetString(), buffer.GetLength()};
}

std::string ToStableString(const Value& doc) {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer writer(buffer);
  AcceptStable(doc.GetNative(), writer);
  return std::string{buffer.GetString(), buffer.GetLength()};
}

logging::LogHelper& operator<<(logging::LogHelper& lh, const Value& doc) {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer writer(buffer);
  doc.GetNative().Accept(writer);
  return lh << std::string_view{buffer.GetString(), buffer.GetLength()};
}

namespace blocking {

Value FromFile(const std::string& path) {
  std::ifstream is(path);
  try {
    return FromStream(is);
  } catch (const std::exception& e) {
    throw ParseException(
        fmt::format("Parsing '{}' failed. {}", path, e.what()));
  }
}

}  // namespace blocking

namespace impl {

struct StringBuffer::Impl final {
  rapidjson::StringBuffer buffer;
};

StringBuffer::StringBuffer(const formats::json::Value& value) {
  rapidjson::Writer writer(pimpl_->buffer);
  value.GetNative().Accept(writer);
}

StringBuffer::~StringBuffer() = default;

std::string_view StringBuffer::GetStringView() const {
  return std::string_view{pimpl_->buffer.GetString(),
                          pimpl_->buffer.GetLength()};
}

}  // namespace impl

}  // namespace formats::json

USERVER_NAMESPACE_END

auto fmt::formatter<USERVER_NAMESPACE::formats::json::Value>::parse(
    format_parse_context& ctx) -> decltype(ctx.begin()) {
  return ctx.begin();
}
