
#include <userver/chaotic/type_bundle_cpp.hpp>

#include "oneofdiscriminator.hpp"
#include "oneofdiscriminator_parsers.ipp"
#include "oneofdiscriminator_sax_parsers.hpp"

namespace ns {

A FromJsonString(std::string_view json, USERVER_NAMESPACE::formats::parse::To<A>) {
  return USERVER_NAMESPACE::formats::json::parser::ParseToType<
      A, USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<USERVER_NAMESPACE::chaotic::sax::Parser<A>>>(json);
}

std::string ToJsonString(const A& value) {
  USERVER_NAMESPACE::formats::json::StringBuilder builder;
  WriteToStream(value, builder);
  return builder.GetString();
}

bool operator==(const A& lhs, const A& rhs) {
  return lhs.type == rhs.type && lhs.a_prop == rhs.a_prop && lhs.extra == rhs.extra &&

         true;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const A& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

A Parse(USERVER_NAMESPACE::formats::json::Value json, USERVER_NAMESPACE::formats::parse::To<A> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

A Parse(USERVER_NAMESPACE::formats::yaml::Value json, USERVER_NAMESPACE::formats::parse::To<A> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

A Parse(USERVER_NAMESPACE::yaml_config::Value json, USERVER_NAMESPACE::formats::parse::To<A> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const A& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = value.extra;

  if (value.type) {
    vb["type"] = USERVER_NAMESPACE::chaotic::Primitive<std::string>{*value.type};
  }

  if (value.a_prop) {
    vb["a_prop"] = USERVER_NAMESPACE::chaotic::Primitive<int>{*value.a_prop};
  }

  return vb.ExtractValue();
}

void WriteToStream([[maybe_unused]] const ::ns::A& value, USERVER_NAMESPACE::formats::json::StringBuilder& sw,
                   [[maybe_unused]] bool hide_brackets, [[maybe_unused]] std::string_view hide_field_name) {
  std::optional<USERVER_NAMESPACE::formats::json::StringBuilder::ObjectGuard> guard;
  if (!hide_brackets) guard.emplace(sw);

  if (value.type && hide_field_name != "type") {
    sw.Key("type");
    WriteToStream(USERVER_NAMESPACE::chaotic::Primitive<std::string>{*value.type}, sw);
  }

  if (value.a_prop && hide_field_name != "a_prop") {
    sw.Key("a_prop");
    WriteToStream(USERVER_NAMESPACE::chaotic::Primitive<int>{*value.a_prop}, sw);
  }

  for (const auto& [field_key, field_value] : USERVER_NAMESPACE::formats::common::Items(value.extra)) {
    sw.Key(field_key);
    WriteToStream(field_value, sw);
  }
}

B FromJsonString(std::string_view json, USERVER_NAMESPACE::formats::parse::To<B>) {
  return USERVER_NAMESPACE::formats::json::parser::ParseToType<
      B, USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<USERVER_NAMESPACE::chaotic::sax::Parser<B>>>(json);
}

std::string ToJsonString(const B& value) {
  USERVER_NAMESPACE::formats::json::StringBuilder builder;
  WriteToStream(value, builder);
  return builder.GetString();
}

bool operator==(const B& lhs, const B& rhs) {
  return lhs.type == rhs.type && lhs.b_prop == rhs.b_prop && lhs.extra == rhs.extra &&

         true;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const B& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

B Parse(USERVER_NAMESPACE::formats::json::Value json, USERVER_NAMESPACE::formats::parse::To<B> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

B Parse(USERVER_NAMESPACE::formats::yaml::Value json, USERVER_NAMESPACE::formats::parse::To<B> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

B Parse(USERVER_NAMESPACE::yaml_config::Value json, USERVER_NAMESPACE::formats::parse::To<B> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const B& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = value.extra;

  if (value.type) {
    vb["type"] = USERVER_NAMESPACE::chaotic::Primitive<std::string>{*value.type};
  }

  if (value.b_prop) {
    vb["b_prop"] = USERVER_NAMESPACE::chaotic::Primitive<int>{*value.b_prop};
  }

  return vb.ExtractValue();
}

void WriteToStream([[maybe_unused]] const ::ns::B& value, USERVER_NAMESPACE::formats::json::StringBuilder& sw,
                   [[maybe_unused]] bool hide_brackets, [[maybe_unused]] std::string_view hide_field_name) {
  std::optional<USERVER_NAMESPACE::formats::json::StringBuilder::ObjectGuard> guard;
  if (!hide_brackets) guard.emplace(sw);

  if (value.type && hide_field_name != "type") {
    sw.Key("type");
    WriteToStream(USERVER_NAMESPACE::chaotic::Primitive<std::string>{*value.type}, sw);
  }

  if (value.b_prop && hide_field_name != "b_prop") {
    sw.Key("b_prop");
    WriteToStream(USERVER_NAMESPACE::chaotic::Primitive<int>{*value.b_prop}, sw);
  }

  for (const auto& [field_key, field_value] : USERVER_NAMESPACE::formats::common::Items(value.extra)) {
    sw.Key(field_key);
    WriteToStream(field_value, sw);
  }
}

C FromJsonString(std::string_view json, USERVER_NAMESPACE::formats::parse::To<C>) {
  return USERVER_NAMESPACE::formats::json::parser::ParseToType<
      C, USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<USERVER_NAMESPACE::chaotic::sax::Parser<C>>>(json);
}

std::string ToJsonString(const C& value) {
  USERVER_NAMESPACE::formats::json::StringBuilder builder;
  WriteToStream(value, builder);
  return builder.GetString();
}

bool operator==(const C& lhs, const C& rhs) { return lhs.version == rhs.version && true; }

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const C& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

C Parse(USERVER_NAMESPACE::formats::json::Value json, USERVER_NAMESPACE::formats::parse::To<C> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

C Parse(USERVER_NAMESPACE::formats::yaml::Value json, USERVER_NAMESPACE::formats::parse::To<C> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

C Parse(USERVER_NAMESPACE::yaml_config::Value json, USERVER_NAMESPACE::formats::parse::To<C> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const C& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = USERVER_NAMESPACE::formats::common::Type::kObject;

  if (value.version) {
    vb["version"] = USERVER_NAMESPACE::chaotic::Primitive<int>{*value.version};
  }

  return vb.ExtractValue();
}

void WriteToStream([[maybe_unused]] const ::ns::C& value, USERVER_NAMESPACE::formats::json::StringBuilder& sw,
                   [[maybe_unused]] bool hide_brackets, [[maybe_unused]] std::string_view hide_field_name) {
  std::optional<USERVER_NAMESPACE::formats::json::StringBuilder::ObjectGuard> guard;
  if (!hide_brackets) guard.emplace(sw);

  if (value.version && hide_field_name != "version") {
    sw.Key("version");
    WriteToStream(USERVER_NAMESPACE::chaotic::Primitive<int>{*value.version}, sw);
  }
}

D FromJsonString(std::string_view json, USERVER_NAMESPACE::formats::parse::To<D>) {
  return USERVER_NAMESPACE::formats::json::parser::ParseToType<
      D, USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<USERVER_NAMESPACE::chaotic::sax::Parser<D>>>(json);
}

std::string ToJsonString(const D& value) {
  USERVER_NAMESPACE::formats::json::StringBuilder builder;
  WriteToStream(value, builder);
  return builder.GetString();
}

bool operator==(const D& lhs, const D& rhs) { return lhs.version == rhs.version && true; }

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const D& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

D Parse(USERVER_NAMESPACE::formats::json::Value json, USERVER_NAMESPACE::formats::parse::To<D> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

D Parse(USERVER_NAMESPACE::formats::yaml::Value json, USERVER_NAMESPACE::formats::parse::To<D> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

D Parse(USERVER_NAMESPACE::yaml_config::Value json, USERVER_NAMESPACE::formats::parse::To<D> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const D& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = USERVER_NAMESPACE::formats::common::Type::kObject;

  if (value.version) {
    vb["version"] = USERVER_NAMESPACE::chaotic::Primitive<int>{*value.version};
  }

  return vb.ExtractValue();
}

void WriteToStream([[maybe_unused]] const ::ns::D& value, USERVER_NAMESPACE::formats::json::StringBuilder& sw,
                   [[maybe_unused]] bool hide_brackets, [[maybe_unused]] std::string_view hide_field_name) {
  std::optional<USERVER_NAMESPACE::formats::json::StringBuilder::ObjectGuard> guard;
  if (!hide_brackets) guard.emplace(sw);

  if (value.version && hide_field_name != "version") {
    sw.Key("version");
    WriteToStream(USERVER_NAMESPACE::chaotic::Primitive<int>{*value.version}, sw);
  }
}

IntegerOneOfDiscriminator FromJsonString(std::string_view json,
                                         USERVER_NAMESPACE::formats::parse::To<IntegerOneOfDiscriminator>) {
  return USERVER_NAMESPACE::formats::json::parser::ParseToType<
      IntegerOneOfDiscriminator, USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<
                                     USERVER_NAMESPACE::chaotic::sax::Parser<IntegerOneOfDiscriminator>>>(json);
}

std::string ToJsonString(const IntegerOneOfDiscriminator& value) {
  USERVER_NAMESPACE::formats::json::StringBuilder builder;
  WriteToStream(value, builder);
  return builder.GetString();
}

bool operator==(const IntegerOneOfDiscriminator& lhs, const IntegerOneOfDiscriminator& rhs) {
  return lhs.foo == rhs.foo && true;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const IntegerOneOfDiscriminator& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

IntegerOneOfDiscriminator Parse(USERVER_NAMESPACE::formats::json::Value json,
                                USERVER_NAMESPACE::formats::parse::To<IntegerOneOfDiscriminator> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

/* Parse(USERVER_NAMESPACE::formats::yaml::Value, To<IntegerOneOfDiscriminator>) was not generated:
 * ::ns::IntegerOneOfDiscriminator::Foo has JSON-specific field "extra" */

/* Parse(USERVER_NAMESPACE::yaml_config::Value, To<IntegerOneOfDiscriminator>) was not generated:
 * ::ns::IntegerOneOfDiscriminator::Foo has JSON-specific field "extra" */

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const IntegerOneOfDiscriminator& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = USERVER_NAMESPACE::formats::common::Type::kObject;

  if (value.foo) {
    vb["foo"] =
        USERVER_NAMESPACE::chaotic::OneOfWithDiscriminator<&::ns::IntegerOneOfDiscriminator::kFoo_Settings,
                                                           USERVER_NAMESPACE::chaotic::Primitive<::ns::C>,
                                                           USERVER_NAMESPACE::chaotic::Primitive<::ns::D>>{*value.foo};
  }

  return vb.ExtractValue();
}

void WriteToStream([[maybe_unused]] const ::ns::IntegerOneOfDiscriminator& value,
                   USERVER_NAMESPACE::formats::json::StringBuilder& sw, [[maybe_unused]] bool hide_brackets,
                   [[maybe_unused]] std::string_view hide_field_name) {
  std::optional<USERVER_NAMESPACE::formats::json::StringBuilder::ObjectGuard> guard;
  if (!hide_brackets) guard.emplace(sw);

  if (value.foo && hide_field_name != "foo") {
    sw.Key("foo");
    WriteToStream(
        USERVER_NAMESPACE::chaotic::OneOfWithDiscriminator<&::ns::IntegerOneOfDiscriminator::kFoo_Settings,
                                                           USERVER_NAMESPACE::chaotic::Primitive<::ns::C>,
                                                           USERVER_NAMESPACE::chaotic::Primitive<::ns::D>>{*value.foo},
        sw);
  }
}

OneOfDiscriminator FromJsonString(std::string_view json, USERVER_NAMESPACE::formats::parse::To<OneOfDiscriminator>) {
  return USERVER_NAMESPACE::formats::json::parser::ParseToType<
      OneOfDiscriminator, USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<
                              USERVER_NAMESPACE::chaotic::sax::Parser<OneOfDiscriminator>>>(json);
}

std::string ToJsonString(const OneOfDiscriminator& value) {
  USERVER_NAMESPACE::formats::json::StringBuilder builder;
  WriteToStream(value, builder);
  return builder.GetString();
}

bool operator==(const OneOfDiscriminator& lhs, const OneOfDiscriminator& rhs) { return lhs.foo == rhs.foo && true; }

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const OneOfDiscriminator& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

OneOfDiscriminator Parse(USERVER_NAMESPACE::formats::json::Value json,
                         USERVER_NAMESPACE::formats::parse::To<OneOfDiscriminator> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

/* Parse(USERVER_NAMESPACE::formats::yaml::Value, To<OneOfDiscriminator>) was not generated:
 * ::ns::OneOfDiscriminator::Foo has JSON-specific field "extra" */

/* Parse(USERVER_NAMESPACE::yaml_config::Value, To<OneOfDiscriminator>) was not generated: ::ns::OneOfDiscriminator::Foo
 * has JSON-specific field "extra" */

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const OneOfDiscriminator& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = USERVER_NAMESPACE::formats::common::Type::kObject;

  if (value.foo) {
    vb["foo"] =
        USERVER_NAMESPACE::chaotic::OneOfWithDiscriminator<&::ns::OneOfDiscriminator::kFoo_Settings,
                                                           USERVER_NAMESPACE::chaotic::Primitive<::ns::A>,
                                                           USERVER_NAMESPACE::chaotic::Primitive<::ns::B>>{*value.foo};
  }

  return vb.ExtractValue();
}

void WriteToStream([[maybe_unused]] const ::ns::OneOfDiscriminator& value,
                   USERVER_NAMESPACE::formats::json::StringBuilder& sw, [[maybe_unused]] bool hide_brackets,
                   [[maybe_unused]] std::string_view hide_field_name) {
  std::optional<USERVER_NAMESPACE::formats::json::StringBuilder::ObjectGuard> guard;
  if (!hide_brackets) guard.emplace(sw);

  if (value.foo && hide_field_name != "foo") {
    sw.Key("foo");
    WriteToStream(
        USERVER_NAMESPACE::chaotic::OneOfWithDiscriminator<&::ns::OneOfDiscriminator::kFoo_Settings,
                                                           USERVER_NAMESPACE::chaotic::Primitive<::ns::A>,
                                                           USERVER_NAMESPACE::chaotic::Primitive<::ns::B>>{*value.foo},
        sw);
  }
}

}  // namespace ns

