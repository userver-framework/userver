#include <storages/postgres/io/user_types.hpp>

#include <logging/log.hpp>

namespace storages::postgres {

namespace {

using ParserMap = std::unordered_multimap<DBTypeName, std::string>;

ParserMap& BinaryParsers() {
  static ParserMap parsers_;
  return parsers_;
}

ParserMap& TextParsers() {
  static ParserMap parsers_;
  return parsers_;
}

}  // namespace

Oid UserTypes::FindOid(DBTypeName name) const {
  if (auto f = by_name_.find(name); f != by_name_.end()) {
    return f->second->oid;
  }
  LOG_WARNING() << "PostgreSQL type " << name.schema << "." << name.name
                << " not found";
  return static_cast<Oid>(io::PredefinedOids::kInvalid);
}

Oid UserTypes::FindArrayOid(DBTypeName name) const {
  if (auto f = by_name_.find(name); f != by_name_.end()) {
    return f->second->array_type;
  }
  LOG_WARNING() << "PostgreSQL type " << name.schema << "." << name.name
                << " not found";
  return static_cast<Oid>(io::PredefinedOids::kInvalid);
}

DBTypeName UserTypes::FindName(Oid oid) const {
  if (auto f = by_oid_.find(oid); f != by_oid_.end()) {
    return f->second->GetName();
  }
  LOG_WARNING() << "PostgreSQL type with oid " << oid << " not found";
  return {};
}

DBTypeName UserTypes::FindBaseName(Oid oid) const {
  using TypeCategory = DBTypeDescription::TypeCategory;
  using TypeClass = DBTypeDescription::TypeClass;

  if (auto f = by_oid_.find(oid); f != by_oid_.end()) {
    // We have a type description
    const auto& desc = *f->second;
    if (desc.type_class == TypeClass::kDomain) {
      return FindBaseName(desc.base_type);
    } else if (desc.category == TypeCategory::kArray) {
      return FindBaseName(desc.element_type);
    }
    return f->second->GetName();
  }
  LOG_WARNING() << "PostgreSQL type with oid " << oid << " not found";
  return {};
}

Oid UserTypes::FindBaseOid(Oid oid) const {
  using TypeCategory = DBTypeDescription::TypeCategory;
  using TypeClass = DBTypeDescription::TypeClass;
  if (auto f = by_oid_.find(oid); f != by_oid_.end()) {
    // We have a type description
    const auto& desc = *f->second;
    if (desc.type_class == TypeClass::kDomain) {
      return FindBaseOid(desc.base_type);
    } else if (desc.category == TypeCategory::kArray) {
      return FindBaseOid(desc.element_type);
    }
    return desc.oid;
  }
  LOG_WARNING() << "PostgreSQL user type with oid " << oid << " not found";
  return oid;
}

Oid UserTypes::FindBaseOid(DBTypeName name) const {
  auto oid = FindOid(name);
  return FindBaseOid(oid);
}

bool UserTypes::HasBinaryParser(Oid oid) const {
  auto name = FindBaseName(oid);
  if (!name.Empty()) {
    return io::HasBinaryParser(name);
  }
  return false;
}

bool UserTypes::HasTextParser(Oid oid) const {
  auto name = FindBaseName(oid);
  if (!name.Empty()) {
    return io::HasTextParser(name);
  }
  return false;
}

void UserTypes::Reset() {
  types_.clear();
  by_oid_.clear();
  by_name_.clear();
}

void UserTypes::AddType(DBTypeDescription&& desc) {
  auto oid = desc.oid;
  LOG_DEBUG() << "User type " << oid << " " << desc.schema << "." << desc.name
              << " class: " << static_cast<char>(desc.type_class)
              << " category: " << static_cast<char>(desc.category);

  if (auto ins = types_.insert(std::move(desc)); ins.second) {
    by_oid_.insert(std::make_pair(oid, ins.first));
    by_name_.insert(std::make_pair(ins.first->GetName(), ins.first));
  } else {
    // schema and name is not available any more as it was moved
    LOG_ERROR() << "Failed to insert user type with oid " << oid;
  }
}

namespace io {

bool HasTextParser(DBTypeName name) { return TextParsers().count(name) > 0; }

bool HasBinaryParser(DBTypeName name) {
  return BinaryParsers().count(name) > 0;
}

namespace detail {

RegisterUserTypeParser RegisterUserTypeParser::Register(
    const DBTypeName& pg_name, std::string&& cpp_name, bool text_parser,
    bool bin_parser) {
  auto both = text_parser && bin_parser;
  if (both) {
    TextParsers().insert(std::make_pair(pg_name, cpp_name));
    BinaryParsers().insert(std::make_pair(pg_name, std::move(cpp_name)));
  } else if (text_parser) {
    TextParsers().insert(std::make_pair(pg_name, std::move(cpp_name)));
  } else if (bin_parser) {
    BinaryParsers().insert(std::make_pair(pg_name, std::move(cpp_name)));
  }
  return {};
}

}  // namespace detail

}  // namespace io

}  // namespace storages::postgres
