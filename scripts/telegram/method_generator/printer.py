import re

INDENT_1 = "  "

def add_indent(lines, indent):
  for i in range(len(lines)):
    if lines[i] != "":
      lines[i] = indent + lines[i]

  return lines


def camel_to_snake(name):
    name = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
    return re.sub('([a-z0-9])([A-Z])', r'\1_\2', name).lower()


INCLUDE_PUBLIC_PATH_USERVER_TELEGRAM = "userver/telegram"
INCLUDE_PRIVATE_PATH_USERVER_TELEGRAM = "telegram"
INCLUDE_TYPES_SUFFIX = "/bot/types"
INCLUDE_REQUESTS_SUFFIX = "/bot/requests"

INCLUDE_PUBLIC_PATH_USERVER_TELEGRAM_TYPES = INCLUDE_PUBLIC_PATH_USERVER_TELEGRAM + INCLUDE_TYPES_SUFFIX
INCLUDE_PRIVATE_PATH_USERVER_TELEGRAM_TYPES = INCLUDE_PRIVATE_PATH_USERVER_TELEGRAM + INCLUDE_TYPES_SUFFIX
INCLUDE_PUBLIC_PATH_USERVER_TELEGRAM_REQUESTS = INCLUDE_PUBLIC_PATH_USERVER_TELEGRAM + INCLUDE_REQUESTS_SUFFIX
INCLUDE_PRIVATE_PATH_USERVER_TELEGRAM_REQUESTS = INCLUDE_PRIVATE_PATH_USERVER_TELEGRAM + INCLUDE_REQUESTS_SUFFIX

class IncludesPrinter:
  def __init__(self, obj):
    self.obj = obj

  def pragma_once(self):
    return "#pragma once"

  def hpp_telegram_includes(self):
    includes = ["#include <userver/telegram/bot/requests/request.hpp>"]
    for field in self.obj.fields:
      if field.type_is_struct and field.base_cpptype != self.obj.name:
        includes.append(
          "#include <{}/{}.hpp>".format(
            INCLUDE_PUBLIC_PATH_USERVER_TELEGRAM_TYPES,
            camel_to_snake(field.base_cpptype)
          )
        )
    
    includes = list(set(includes))
    includes.sort()
    return includes


  def header_base_includes(self):
    includes = []
    for field in self.obj.fields:
      field_type = field.full_cpp_type
      if field_type.find("std::int64_t") != -1:
        includes.append("#include <cstdint>")
      if field_type.find("std::string") != -1:
        includes.append("#include <string>")
      if field_type.find("std::vector") != -1:
        includes.append("#include <vector>")
      if field_type.find("std::optional") != -1:
        includes.append("#include <optional>")
      if field_type.find("std::unique_ptr") != -1:
        includes.append("#include <memory>")
      if field_type.find("std::chrono") != -1:
        includes.append("#include <chrono>")

    includes = list(set(includes))
    includes.sort()

    return includes


  def header_userver_includes(self):
    return ["#include <userver/formats/json_fwd.hpp>"]

  def header_includes(self):
    includes = [self.pragma_once()]
    includes.append("")

    tg_incl = self.hpp_telegram_includes()
    if len(tg_incl) != 0:
      includes += tg_incl
      includes.append("")

    base_incl = self.header_base_includes()
    if len(base_incl) != 0:
      includes += base_incl
      includes.append("")

    includes += self.header_userver_includes()
    return includes

  def cpp_header_include(self):
    return [
      "#include <{}/{}.hpp>".format(
        INCLUDE_PUBLIC_PATH_USERVER_TELEGRAM_REQUESTS,
        camel_to_snake(self.obj.name)
      )
    ]

  def cpp_tg_includes(self):
    includes = ["#include <{}/bot/requests/request_data.hpp>".format(INCLUDE_PRIVATE_PATH_USERVER_TELEGRAM)]
    for field in self.obj.fields:
      if field.full_cpp_type.find("std::unique_ptr") != -1:
        includes += [
          "#include <{}/bot/formats/parse.hpp>".format(INCLUDE_PRIVATE_PATH_USERVER_TELEGRAM),
          "#include <{}/bot/formats/serialize.hpp>".format(INCLUDE_PRIVATE_PATH_USERVER_TELEGRAM)
        ]

      if field.full_cpp_type.find("std::chrono::system_clock::time_point") != -1:
        includes += [
          "#include <{}/time.hpp>".format(INCLUDE_PRIVATE_PATH_USERVER_TELEGRAM_TYPES),
        ]
      if field.is_optional:
        includes += [
          "#include <{}/bot/formats/value_builder.hpp>".format(INCLUDE_PRIVATE_PATH_USERVER_TELEGRAM),
        ]

    includes = list(set(includes))

    includes.sort()
    return includes

  def cpp_userver_includes(self):
    includes = ["#include <userver/formats/json.hpp>"]
    for field in self.obj.fields:
      if field.full_cpp_type.find("std::optional") != -1 or field.full_cpp_type.find("std::vector") != -1:
        includes += [
          "#include <userver/formats/parse/common_containers.hpp>",
          "#include <userver/formats/serialize/common_containers.hpp>"
        ]
      if field.full_cpp_type.find("std::chrono") != -1:
        includes += [
          "#include <userver/formats/json/serialize_duration.hpp>",
          "#include <userver/formats/parse/common.hpp>"
        ]
    
    includes = list(set(includes))
    includes.sort()
    return includes

  def cpp_includes(self):
    includes = self.cpp_header_include()
    includes.append("")

    ps_incl = self.cpp_tg_includes()
    if len(ps_incl) != 0:
      includes += ps_incl
      includes.append("")

    includes += self.cpp_userver_includes()

    return includes


class NamespacePrinter:
  def __init__(self):
    pass

  def begin_lines(self):
    return [
      "USERVER_NAMESPACE_BEGIN",
      "",
      "namespace telegram::bot {",
    ]

  def end_lines(self):
    return [
      "}  // namespace telegram::bot",
      "",
      "USERVER_NAMESPACE_END"
    ]

  def impl_begin_lines(self):
    return ["namespace impl {"]
  
  def impl_end_lines(self):
    return ["}  // namespace impl"]


class FieldPrinter:
  def __init__(self, field):
    self.field = field

  def struct_field(self):
    if self.field.is_simple:
      return "{} {}{{}};".format(self.field.full_cpp_type, self.field.name)
    else:
      return "{} {};".format(self.field.full_cpp_type, self.field.name)

  def brief(self):
    return "/// @brief {}".format(self.field.description)

  def parse_line(self):
    if self.field.full_cpp_type.find("std::chrono::system_clock::time_point") == -1:
      return "data[\"{}\"].template As<{}>()".format(self.field.name, self.field.full_cpp_type)
    else:
      return "TransformToTimePoint(data[\"{}\"].template As<{}>())".format(self.field.name, self.field.full_cpp_type)

  def parse_set_line(self):
    return "parameters.{} = {}".format(self.field.name, self.parse_line())

  def serialize_line(self, obj_name):
    field_str = ""
    if self.field.full_cpp_type.find("std::chrono::system_clock::time_point") == -1:
      field_str = "{}.{}".format(obj_name, self.field.name)
    else:
      field_str = "TransformToSeconds({}.{})".format(obj_name, self.field.name)
    
    if self.field.is_optional:
      return "SetIfNotNull(builder, \"{}\", {})".format(self.field.name, field_str)
    else:
      return "builder[\"{}\"] = {}".format(self.field.name, field_str)

  def constructor_args(self):
    return "{} _{}".format(self.field.full_cpp_type, self.field.name)
  
  def constructor_init(self):
    if self.field.is_simple:
      return "{}(_{})".format(self.field.name, self.field.name)
    else:
      return "{}(std::move(_{}))".format(self.field.name, self.field.name)

  def struct_fields_lines(self):
    return [
      self.brief(),
      self.struct_field()
    ]


class StructPrinter:
  class ParametersPrinter:
    def __init__(self, obj):
      self.obj = obj
    
    def field_lines(self):
      lines = []

      for i in range(len(self.obj.fields)):
        field = self.obj.fields[i]
        field_printer = FieldPrinter(field)

        lines += field_printer.struct_fields_lines()
        if i + 1 != len(self.obj.fields):
          lines.append("")

      return lines

    def constructor_declaration_lines(self):
      required_fields = []
      for field in self.obj.fields:
        if field.is_required:
          required_fields.append(field)

      lines = []
      if len(required_fields) == 0:
        return lines

      contructor_pref = "Parameters("
      for i in range(len(required_fields)):
        field = required_fields[i]
        field_printer = FieldPrinter(field)

        prefix = " " * len(contructor_pref)
        if i == 0:
          prefix = contructor_pref

        suffix = ","
        if i + 1 == len(required_fields):
          suffix = ");"

        lines.append(prefix + field_printer.constructor_args() + suffix)
      
      return lines
    
    def constructor_realization_lines(self):
      required_fields = []
      for field in self.obj.fields:
        if field.is_required:
          required_fields.append(field)

      lines = []
      if len(required_fields) == 0:
        return lines

      contructor_pref = "{}::Parameters::Parameters(".format(self.obj.method_name)
      for i in range(len(required_fields)):
        field = required_fields[i]
        field_printer = FieldPrinter(field)

        prefix = " " * len(contructor_pref)
        if i == 0:
          prefix = contructor_pref

        suffix = ","
        if i + 1 == len(required_fields):
          suffix = ")"

        lines.append(prefix + field_printer.constructor_args() + suffix)
      
      for i in range(len(required_fields)):
        field = required_fields[i]
        field_printer = FieldPrinter(field)

        prefix = "  , "
        if i == 0:
          prefix = "  : "

        suffix = ""
        if i + 1 == len(required_fields):
          suffix = " {}"

        lines.append(prefix + field_printer.constructor_init() + suffix)
      
      return lines

    def struct_begin(self):
      return "struct Parameters{"
    
    def struct_end(self):
      return "};"
  
    def parameters_lines(self):
      lines = []
      lines.append(self.struct_begin())

      cstr = self.constructor_declaration_lines()
      if len(cstr) != 0:
        lines += add_indent(cstr, INDENT_1)
        lines.append("")
      
      field_lines = add_indent(self.field_lines(), INDENT_1)
      if len(field_lines) != 0:
        lines += field_lines

      lines.append(self.struct_end())
      return lines

  def __init__(self, obj):
    self.obj = obj
    self.parameters_printer = self.ParametersPrinter(obj)

  def brief(self):
    return "/// @brief {}".format(self.obj.description)
  
  def see(self):
    return "/// @see https://core.telegram.org/bots/api#{}".format(camel_to_snake(self.obj.name).replace('_', ''))

  def method_k_name(self):
    return "static constexpr std::string_view kName = \"{}\";".format(self.obj.base_name)
  
  def http_method(self):
    if self.obj.name.find("Get") != -1:
      return "static constexpr auto kHttpMethod = clients::http::HttpMethod::kGet;"
    else:
      return "static constexpr auto kHttpMethod = clients::http::HttpMethod::kPost;"

  def declaration_fill_request_data(self):
    prefix = "static void FillRequestData("
    return [
      prefix + "clients::http::Request& request,",
      " " * len(prefix) + "const Parameters& parameters);"
    ]
  
  def realization_fill_request_data(self):
    prefix = "void {}::FillRequestData(".format(self.obj.method_name)
    return [
      prefix + "clients::http::Request& request,",
      " " * len(prefix) + "const Parameters& parameters) {",
      "  FillRequestDataAsJson<{}>(request, parameters);".format(self.obj.method_name),
      "}"
    ]
  
  def declaration_parse_response_data(self):
    return ["static Reply ParseResponseData(clients::http::Response& response);"]
  
  def realization_parse_response_data(self):
    return [
      "{}::Reply {}::ParseResponseData(".format(self.obj.method_name, self.obj.method_name),
      "    clients::http::Response& response) {",
      "  return ParseResponseDataFromJson<{}>(response);".format(self.obj.method_name),
      "}"
    ]
  
  def using_reply(self):
    return "using Reply = ;"

  def struct_begin(self):
    return "struct {} {{".format(self.obj.method_name)

  def struct_end(self):
    return "};"

  def struct_lines(self):
    lines = []
    lines.append(self.brief())
    lines.append(self.see())
    lines.append(self.struct_begin())
    lines.append(INDENT_1 + self.method_k_name())
    lines.append("")
    lines.append(INDENT_1 + self.http_method())
    lines.append("")
    lines += add_indent(self.parameters_printer.parameters_lines(), INDENT_1)
    lines.append("")
    lines.append(INDENT_1 + self.using_reply())
    lines.append("")
    lines += add_indent(self.declaration_fill_request_data(), INDENT_1)
    lines.append("")
    lines += add_indent(self.declaration_parse_response_data(), INDENT_1)
    lines.append(self.struct_end())
    return lines

  def struct_impl_lines(self):
    lines = []
    ctr_lines = self.parameters_printer.constructor_realization_lines()
    if len(ctr_lines) != 0:
      lines += ctr_lines
      lines.append("")
  
    lines += self.realization_fill_request_data()
    lines.append("")
    lines += self.realization_parse_response_data()
    
    return lines


class ParseFuncPrinter:
  def __init__(self, obj):
    self.obj = obj

  def func_declaration_lines(self):
    func_start = "{}::Parameters Parse".format(self.obj.method_name)
    lines = [func_start + "(const formats::json::Value& json,"]
    lines.append(" " * (len(func_start) + 1) + "formats::parse::To<{}::Parameters>);".format(self.obj.method_name))
    return lines

  def func_realization_lines(self):
    func_start = "{}::Parameters Parse".format(self.obj.method_name)
    lines = [func_start + "(const formats::json::Value& json,"]
    lines.append(" " * (len(func_start) + 1) + "formats::parse::To<{}::Parameters> to) {{".format(self.obj.method_name))
    lines.append(INDENT_1 + "return impl::Parse(json, to);")
    lines.append("}")
    return lines

  def func_impl_lines(self):
    lines = ["template <class Value>"]
    lines.append("{}::Parameters Parse(const Value& data, formats::parse::To<{}::Parameters>) {{".format( \
      self.obj.method_name, self.obj.method_name))

    default_constructor = True
    for field in self.obj.fields:
      default_constructor = default_constructor and not field.is_required

    if default_constructor:
      parse_lines = []
      for i in range(len(self.obj.fields)):
        field = self.obj.fields[i]
        field_printer = FieldPrinter(field)

        parse_lines.append(field_printer.parse_line())
        if i + 1 != len(self.obj.fields):
          parse_lines[-1] += ","
      
      obj_lines = ["return {}::Parameters{{".format(self.obj.method_name)]
      obj_lines += add_indent(parse_lines, INDENT_1)
      obj_lines.append("};")

      lines += add_indent(obj_lines, INDENT_1)
    else:
      required_lines = []
      optional_lines = []
      for field in self.obj.fields:
        field_printer = FieldPrinter(field)
        if field.is_required:
          required_lines.append(field_printer.parse_line())
        else:
          optional_lines.append(field_printer.parse_set_line() + ";")
      
      for i in range(len(required_lines)):
        if i + 1 != len(required_lines):
          required_lines[i] += ","

      obj_lines = ["{}::Parameters parameters{{".format(self.obj.method_name)]
      obj_lines += add_indent(required_lines, INDENT_1)
      obj_lines.append("};")
      obj_lines += optional_lines
      obj_lines.append("return parameters;")

      lines += add_indent(obj_lines, INDENT_1)

    lines.append("}")
    return lines  


class SerializeFuncPrinter:
  def __init__(self, obj):
    self.obj = obj

  def func_declaration_lines(self):
    func_start = "formats::json::Value Serialize"
    lines = [func_start + "(const {}::Parameters& parameters,".format(self.obj.method_name)]
    lines.append(" " * (len(func_start) + 1) + "formats::serialize::To<formats::json::Value>);")
    return lines

  def func_realization_lines(self):
    func_start = "formats::json::Value Serialize"
    lines = [func_start + "(const {}::Parameters& parameters,".format(self.obj.method_name)]
    lines.append(" " * (len(func_start) + 1) + "formats::serialize::To<formats::json::Value> to) {")
    lines.append(INDENT_1 + "return impl::Serialize(parameters, to);")
    lines.append("}")
    return lines

  def func_impl_lines(self):
    camel_name = camel_to_snake(self.obj.name)
    lines = ["template <class Value>"]
    lines.append("Value Serialize(const {}::Parameters& parameters, formats::serialize::To<Value>) {{" \
      .format(self.obj.method_name))

    serialize_lines = ["typename Value::Builder builder;"]
    for field in self.obj.fields:
      field_printer = FieldPrinter(field)
      serialize_lines.append(field_printer.serialize_line("parameters") + ";")
    serialize_lines.append("return builder.ExtractValue();")

    lines += add_indent(serialize_lines, INDENT_1)
    lines.append("}")
    return lines

class UsingPrinter:
  def __init__(self, obj):
    self.obj = obj
  
  def header_request_using(self):
    return ["using {}Request = Request<{}>;".format(self.obj.name, self.obj.method_name)]


class ObjFile:
  def __init__(self, obj):
    self.obj = obj
    self.header_file_name = "{}.hpp".format(camel_to_snake(obj.name))
    self.cpp_file_name = "{}.cpp".format(camel_to_snake(obj.name))

    self.includes_printer = IncludesPrinter(self.obj)
    self.namespace_printer = NamespacePrinter()
    self.struct_printer = StructPrinter(self.obj)
    self.parse_func_printer = ParseFuncPrinter(self.obj)
    self.serialize_func_printer = SerializeFuncPrinter(self.obj)
    self.using_printer = UsingPrinter(self.obj)

  def header_file(self):
    lines = []

    lines += self.includes_printer.header_includes()
    lines.append("")

    lines += self.namespace_printer.begin_lines()
    lines.append("")

    lines += self.struct_printer.struct_lines()
    lines.append("")

    lines += self.parse_func_printer.func_declaration_lines()
    lines.append("")

    lines += self.serialize_func_printer.func_declaration_lines()
    lines.append("")

    lines += self.using_printer.header_request_using()
    lines.append("")

    lines += self.namespace_printer.end_lines()
    return "\n".join(lines)

  def cpp_file(self):
    lines = []
  
    lines += self.includes_printer.cpp_includes()
    lines.append("")

    lines += self.namespace_printer.begin_lines()
    lines.append("")

    lines += self.namespace_printer.impl_begin_lines()
    lines.append("")

    lines += self.parse_func_printer.func_impl_lines()
    lines.append("")

    lines += self.serialize_func_printer.func_impl_lines()
    lines.append("")

    lines += self.namespace_printer.impl_end_lines()
    lines.append("")

    struct_impl = self.struct_printer.struct_impl_lines()
    if len(struct_impl) != 0:
      lines += struct_impl
      lines.append("")

    lines += self.parse_func_printer.func_realization_lines()
    lines.append("")

    lines += self.serialize_func_printer.func_realization_lines()
    lines.append("")

    lines += self.namespace_printer.end_lines()
    return "\n".join(lines)

  def write_header_file(self, path):
    with open("{}/{}".format(path, self.header_file_name), "w") as f:
      f.write(self.header_file())

  def write_cpp_file(self, path):
    with open("{}/{}".format(path, self.cpp_file_name), "w") as f:
      f.write(self.cpp_file())


  def write(self, header_path, cpp_path):
    self.write_header_file(header_path)
    self.write_cpp_file(cpp_path)


def print_files(objs, header_path, cpp_path):
  for obj in objs:
    ObjFile(obj).write(header_path, cpp_path)
