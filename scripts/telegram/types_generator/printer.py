import re

INDENT_1 = "  "
INDENT_2 = "    "
INDENT_3 = "      "
INDENT_4 = "        "

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

INCLUDE_PUBLIC_PATH_USERVER_TELEGRAM_TYPES = INCLUDE_PUBLIC_PATH_USERVER_TELEGRAM + INCLUDE_TYPES_SUFFIX
INCLUDE_PRIVATE_PATH_USERVER_TELEGRAM_TYPES = INCLUDE_PRIVATE_PATH_USERVER_TELEGRAM + INCLUDE_TYPES_SUFFIX

class IncludesPrinter:
  def __init__(self, obj):
    self.obj = obj

  def pragma_once(self):
    return "#pragma once"

  def hpp_dependent_types_includes(self):
    includes = []
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

    dep_incl = self.hpp_dependent_types_includes()
    if len(dep_incl) != 0:
      includes += dep_incl
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
        INCLUDE_PUBLIC_PATH_USERVER_TELEGRAM_TYPES,
        camel_to_snake(self.obj.name)
      )
    ]

  def cpp_parse_serialize_includes(self):
    includes = []
    for field in self.obj.fields:
      if field.full_cpp_type.find("std::unique_ptr") != -1:
        includes = [
          "#include <{}/parse.hpp>".format(INCLUDE_PRIVATE_PATH_USERVER_TELEGRAM_TYPES),
          "#include <{}/serialize.hpp>".format(INCLUDE_PRIVATE_PATH_USERVER_TELEGRAM_TYPES)
        ]

      if field.full_cpp_type.find("std::chrono::system_clock::time_point") != -1:
        includes += [
          "#include <{}/time.hpp>".format(INCLUDE_PRIVATE_PATH_USERVER_TELEGRAM_TYPES),
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

    ps_incl = self.cpp_parse_serialize_includes()
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


class ForwardDeclarationPrinter:
  def __init__(self, obj):
    self.obj = obj
  
  def forward_declaration(self):
    lines = []

    for field in self.obj.fields:
      if field.type_is_struct and field.base_cpptype != self.obj.name:
        lines.append("struct {};".format(field.base_cpptype))
    
    lines = list(set(lines))
    lines.sort()
    return lines


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

  def serialize_line(self, obj_name):
    if self.field.full_cpp_type.find("std::chrono::system_clock::time_point") == -1:
      return "builder[\"{}\"] = {}.{}".format(self.field.name, obj_name, self.field.name)
    else:
      return "builder[\"{}\"] = TransformToSeconds({}.{})".format(self.field.name, obj_name, self.field.name)

  def struct_fields_lines(self):
    return [
      self.brief(),
      self.struct_field()
    ]


class StructPrinter:
  def __init__(self, obj):
    self.obj = obj

  def brief(self):
    return "/// @brief {}".format(self.obj.description)
  
  def see(self):
    return "/// @see https://core.telegram.org/bots/api#{}".format(camel_to_snake(self.obj.name).replace('_', ''))

  def field_lines(self):
    lines = []

    for i in range(len(self.obj.fields)):
      field = self.obj.fields[i]
      field_printer = FieldPrinter(field)

      lines += field_printer.struct_fields_lines()
      if i + 1 != len(self.obj.fields):
        lines.append("")

    return lines

  def struct_begin(self):
    return "struct {} {{".format(self.obj.name)

  def struct_end(self):
    return "};"

  def struct_lines(self):
    lines = []
    lines.append(self.brief())
    lines.append(self.see())
    lines.append(self.struct_begin())
    lines += add_indent(self.field_lines(), INDENT_1)
    lines.append(self.struct_end())
    return lines

class ParseFuncPrinter:
  def __init__(self, obj):
    self.obj = obj

  def func_declaration_lines(self):
    func_start = "{} Parse".format(self.obj.name)
    lines = [func_start + "(const formats::json::Value& json,"]
    lines.append(" " * (len(func_start) + 1) + "formats::parse::To<{}>);".format(self.obj.name))
    return lines
  
  def func_realization_lines(self):
    func_start = "{} Parse".format(self.obj.name)
    lines = [func_start + "(const formats::json::Value& json,"]
    lines.append(" " * (len(func_start) + 1) + "formats::parse::To<{}> to) {{".format(self.obj.name))
    lines.append(INDENT_1 + "return impl::Parse(json, to);")
    lines.append("}")
    return lines

  def func_impl_lines(self):
    lines = ["template <class Value>"]
    lines.append("{} Parse(const Value& data, formats::parse::To<{}>) {{".format(self.obj.name, self.obj.name))

    parse_lines = []
    for i in range(len(self.obj.fields)):
      field = self.obj.fields[i]
      field_printer = FieldPrinter(field)

      parse_lines.append(field_printer.parse_line())
      if i + 1 != len(self.obj.fields):
        parse_lines[-1] += ","
    
    obj_lines = ["return {}{{".format(self.obj.name)]
    obj_lines += add_indent(parse_lines, INDENT_1)
    obj_lines.append("};")

    lines += add_indent(obj_lines, INDENT_1)
    lines.append("}")
    return lines


class SerializeFuncPrinter:
  def __init__(self, obj):
    self.obj = obj

  def func_declaration_lines(self):
    func_start = "formats::json::Value Serialize"
    lines = [func_start + "(const {}& {},".format(self.obj.name, camel_to_snake(self.obj.name))]
    lines.append(" " * (len(func_start) + 1) + "formats::serialize::To<formats::json::Value>);")
    return lines

  def func_realization_lines(self):
    camel_name = camel_to_snake(self.obj.name)
    func_start = "formats::json::Value Serialize"
    lines = [func_start + "(const {}& {},".format(self.obj.name, camel_name)]
    lines.append(" " * (len(func_start) + 1) + "formats::serialize::To<formats::json::Value> to) {")
    lines.append(INDENT_1 + "return impl::Serialize({}, to);".format(camel_name))
    lines.append("}")
    return lines

  def func_impl_lines(self):
    camel_name = camel_to_snake(self.obj.name)
    lines = ["template <class Value>"]
    lines.append("Value Serialize(const {}& {}, formats::serialize::To<Value>) {{" \
      .format(self.obj.name, camel_name))

    serialize_lines = ["typename Value::Builder builder;"]
    for field in self.obj.fields:
      field_printer = FieldPrinter(field)
      serialize_lines.append(field_printer.serialize_line(camel_name) + ";")
    serialize_lines.append("return builder.ExtractValue();")

    lines += add_indent(serialize_lines, INDENT_1)
    lines.append("}")
    return lines


class ObjFile:
  def __init__(self, obj):
    self.obj = obj
    self.header_file_name = "{}.hpp".format(camel_to_snake(obj.name))
    self.cpp_file_name = "{}.cpp".format(camel_to_snake(obj.name))

    self.includes_printer = IncludesPrinter(self.obj)
    self.namespace_printer = NamespacePrinter()
    self.fwd_printer = ForwardDeclarationPrinter(self.obj)
    self.struct_printer = StructPrinter(self.obj)
    self.parse_func_printer = ParseFuncPrinter(self.obj)
    self.serialize_func_printer = SerializeFuncPrinter(self.obj)

  def header_file(self):
    lines = []

    lines += self.includes_printer.header_includes()
    lines.append("")

    lines += self.namespace_printer.begin_lines()
    lines.append("")

    # add_lines = self.fwd_printer.forward_declaration()
    # if len(add_lines) != 0:
    #   lines += add_lines
    #   lines.append("")

    lines += self.struct_printer.struct_lines()
    lines.append("")

    lines += self.parse_func_printer.func_declaration_lines()
    lines.append("")

    lines += self.serialize_func_printer.func_declaration_lines()
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
