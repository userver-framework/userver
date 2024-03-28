ArrayPrefix = "Array of "

TypeToCppType = {
  "Integer or String": "std::string",
  "Integer": "std::int64_t",
  "String": "std::string",
  "Boolean": "bool",
  "True": "bool",
  "Float": "double",
  "Float number": "double",
  "Duration": "std::chrono::seconds",
  "Timepoint": "std::chrono::system_clock::time_point",
}

ParametersSectionBegin = "Parameter\tType\tRequired\tDescription\n"
ObjectSectionBegin = "--------------------------------------------------------------------------------\n"

class Field:
  def __init__(self, name, type_, required, description):
    self.name = name
    self.type_ = type_
    self.description = description

    self.type_is_struct = False
    self.base_cpptype = ""
    self.cpptype = ""
    self.full_cpp_type = ""

    self.is_optional = (required == "Optional")
    self.is_required = (required == "Yes")

    self.arrays_count = 0

    self.set_cpp_type()

    self.is_simple = self.full_cpp_type == "bool" or \
                     self.full_cpp_type == "double" or \
                     self.full_cpp_type == "std::int64_t" or \
                     self.full_cpp_type == "std::chrono::seconds" or \
                     self.full_cpp_type == "std::chrono::system_clock::time_point"

  def set_cpp_type(self):
    type_ = self.type_

    while type_.startswith(ArrayPrefix):
      self.arrays_count += 1
      type_ = type_[len(ArrayPrefix):]

    cpptype = ""

    if type_ in TypeToCppType:
      cpptype = TypeToCppType[type_]
    else:
      cpptype = type_
      self.type_is_struct = True

    self.base_cpptype = cpptype

    for i in range(self.arrays_count):
      cpptype = "std::vector<{}>".format(cpptype)

    self.cpptype = cpptype

    if self.type_is_struct and self.arrays_count == 0:
      self.full_cpp_type = "std::unique_ptr<{}>".format(self.cpptype)
    elif self.is_optional:
      self.full_cpp_type = "std::optional<{}>".format(self.cpptype)
    else:
      self.full_cpp_type = self.cpptype


def parse_field(line):
  lines = line.split("\t")
  assert(len(lines) == 4)
  return Field(lines[0].strip(), lines[1].strip(), lines[2].strip(), lines[3].strip())


def parse_fields(lines):
  assert(lines[0] == ParametersSectionBegin)
  lines = lines[1:]

  fields = []

  while len(lines) != 0 and lines[0] != "\n":
    if len(lines[0].split("\t")) != 4:
      break
    fields.append(parse_field(lines[0]))

    lines = lines[1:]

  return fields, lines


class Object:
  def __init__(self, name, description, fields):
    self.base_name = name
    self.name = self.base_name[0].upper() + self.base_name[1:]
    self.method_name = "{}Method".format(self.name)
    self.request_name = "{}Request".format(self.name)
    self.description = description
    self.fields = fields


def parse_section(lines):
  assert(lines[0] == ObjectSectionBegin)
  lines = lines[1:]

  assert(len(lines) >= 2)
  obj_name = lines[0].strip()
  obj_desc = lines[1].strip()
  lines = lines[2:]
  fields = []

  while len(lines) != 0:
    if lines[0] == ParametersSectionBegin:
      fields, lines = parse_fields(lines)
    elif lines[0] == ObjectSectionBegin:
      break
    else:
      lines = lines[1:]

  return Object(obj_name, obj_desc, fields), lines


def parse(lines):
  objs = []
  while len(lines) != 0:
    obj, lines = parse_section(lines)
    objs.append(obj)

  return objs
