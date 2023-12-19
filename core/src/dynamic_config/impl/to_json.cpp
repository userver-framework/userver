#include <userver/dynamic_config/impl/to_json.hpp>

#include <boost/range/counting_range.hpp>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/formats/json/string_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config::impl {

namespace {

template <typename T>
std::string ToJsonStringImpl(const T& value) {
  formats::json::StringBuilder builder;
  WriteToStream(value, builder);
  return builder.GetString();
}

}  // namespace

std::string DoToJsonString(bool value) { return ToJsonStringImpl(value); }

std::string DoToJsonString(double value) { return ToJsonStringImpl(value); }

std::string DoToJsonString(std::uint64_t value) {
  return ToJsonStringImpl(value);
}

std::string DoToJsonString(std::int64_t value) {
  return ToJsonStringImpl(value);
}

std::string DoToJsonString(std::string_view value) {
  return ToJsonStringImpl(value);
}

std::string SingleToDocsMapString(std::string_view name,
                                  std::string_view value) {
  const ConfigDefault array[] = {{name, DefaultAsJsonString{value}}};
  return MultipleToDocsMapString(std::data(array), std::size(array));
}

std::string MultipleToDocsMapString(const ConfigDefault* data,
                                    std::size_t size) {
  formats::json::StringBuilder builder;
  {
    const formats::json::StringBuilder::ObjectGuard guard{builder};
    for (const auto& config : boost::make_iterator_range_n(data, size)) {
      builder.Key(config.name);
      builder.WriteRawString(config.default_json);
    }
  }
  return builder.GetString();
}

}  // namespace dynamic_config::impl

USERVER_NAMESPACE_END
