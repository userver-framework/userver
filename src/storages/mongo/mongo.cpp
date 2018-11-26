#include <storages/mongo/mongo.hpp>

#include <type_traits>

#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/types.hpp>

namespace storages {
namespace mongo {

namespace impl {
namespace {

template <typename Element>
bool IsOneOf(const Element& elem, utils::Flags<ElementKind> kinds) {
  if (!elem) return !!(kinds & ElementKind::kMissing);

  switch (elem.type()) {
    case bsoncxx::type::k_null:
      return !!(kinds & ElementKind::kNull);

    case bsoncxx::type::k_bool:
      return !!(kinds & ElementKind::kBool);

    case bsoncxx::type::k_double:
    case bsoncxx::type::k_int32:
    case bsoncxx::type::k_int64:
      return !!(kinds & ElementKind::kNumber);

    case bsoncxx::type::k_utf8:
    case bsoncxx::type::k_binary:
      return !!(kinds & ElementKind::kString);

    case bsoncxx::type::k_date:
      return !!(kinds & ElementKind::kTimestamp);

    case bsoncxx::type::k_array:
      return !!(kinds & ElementKind::kArray);

    case bsoncxx::type::k_document:
      return !!(kinds & ElementKind::kDocument);

    case bsoncxx::type::k_oid:
      return (kinds & ElementKind::kOid) || (kinds & ElementKind::kString);

    default:;
  }
  return false;
}

template <typename Element>
double ToDouble(const Element& item) {
  if (IsOneOf(item, {ElementKind::kMissing, ElementKind::kNull})) return 0.0;

  auto type = item.type();
  if (type == bsoncxx::type::k_bool) return item.get_bool() ? 1.0 : 0.0;
  if (type == bsoncxx::type::k_double) return item.get_double();
  if (type == bsoncxx::type::k_int32) return item.get_int32();
  if (type == bsoncxx::type::k_int64) return item.get_int64();
  if (type == bsoncxx::type::k_date) return item.get_date().to_int64();
  throw BadType(item, "ToDouble");
}

template <typename Element>
int64_t ToInt64(const Element& item) {
  if (IsOneOf(item, {ElementKind::kMissing, ElementKind::kNull})) return 0;

  auto type = item.type();
  if (type == bsoncxx::type::k_null) return 0;
  if (type == bsoncxx::type::k_bool) return item.get_bool() ? 1 : 0;
  if (type == bsoncxx::type::k_double) return item.get_double();
  if (type == bsoncxx::type::k_int32) return item.get_int32();
  if (type == bsoncxx::type::k_int64) return item.get_int64();
  if (type == bsoncxx::type::k_date) return item.get_date().to_int64();
  throw BadType(item, "ToInt64");
}

template <typename Element>
std::string ToString(const Element& item) {
  if (IsOneOf(item, {ElementKind::kMissing, ElementKind::kNull})) return {};
  if (IsOneOf(item, ElementKind::kBool)) {
    return item.get_bool() ? "true" : "false";
  }

  auto type = item.type();
  if (type == bsoncxx::type::k_double) return std::to_string(ToDouble(item));
  if (IsOneOf(item, ElementKind::kNumber)) return std::to_string(ToInt64(item));
  if (IsOneOf(item, ElementKind::kTimestamp))
    return std::to_string(item.get_date().to_int64());
  if (type == bsoncxx::type::k_utf8) return item.get_utf8().value.to_string();
  if (type == bsoncxx::type::k_oid) return item.get_oid().value.to_string();
  if (type == bsoncxx::type::k_binary) {
    auto binData = item.get_binary();
    if (binData.size < 0) {
      throw MongoError("Invalid binary data size (" +
                       std::to_string(binData.size) + ')');
    }
    return std::string(reinterpret_cast<const char*>(binData.bytes),
                       binData.size);
  }
  throw BadType(item, "ToString");
}

template <typename Element>
std::chrono::system_clock::time_point ToTimePoint(const Element& item) {
  using Clock = std::chrono::system_clock;

  if (!IsOneOf(item, ElementKind::kTimestamp))
    throw BadType(item, "ToTimePoint");

  std::chrono::milliseconds timestamp{item.get_date().to_int64()};
  if (timestamp > Clock::duration::max()) return Clock::time_point::max();
  if (timestamp < Clock::duration::min()) return Clock::time_point::min();
  return Clock::time_point(timestamp);
}

template <typename Element>
bsoncxx::oid ToOid(const Element& item) {
  if (!IsOneOf(item, ElementKind::kOid)) throw BadType(item, "ToOid");
  return item.get_oid().value;
}

template <typename Element>
bool ToBool(const Element& item) {
  if (IsOneOf(item, {ElementKind::kMissing, ElementKind::kNull})) return false;
  if (IsOneOf(item, ElementKind::kBool)) return item.get_bool();
  if (IsOneOf(item, ElementKind::kNumber)) return ToDouble(item) != 0;
  if (IsOneOf(item, ElementKind::kTimestamp)) return true;
  if (IsOneOf(item, ElementKind::kString)) return !ToString(item).empty();
  if (IsOneOf(item, ElementKind::kArray))
    return !item.get_array().value.empty();
  if (IsOneOf(item, ElementKind::kDocument))
    return !item.get_document().value.empty();
  throw BadType(item, "ToBool");
}

template <typename Element>
bsoncxx::array::view ToArray(const Element& item) {
  if (IsOneOf(item, {ElementKind::kMissing, ElementKind::kNull})) return {};
  if (IsOneOf(item, ElementKind::kArray)) return item.get_array();
  throw BadType(item, "ToArray");
}

template <typename Element>
bsoncxx::document::view ToDocument(const Element& item) {
  if (IsOneOf(item, {ElementKind::kMissing, ElementKind::kNull})) return {};
  if (IsOneOf(item, ElementKind::kDocument)) return item.get_document();
  throw BadType(item, "ToDocument");
}

template <typename Element>
std::vector<std::string> ToStringArray(const Element& array_item) {
  std::vector<std::string> result;
  for (const auto& item : ToArray(array_item)) {
    result.push_back(ToString(item));
  }
  return result;
}

std::string MakeElementDescription(const DocumentElement& item) {
  return "item '" + item.key().to_string() + '\'';
}

std::string MakeElementDescription(const ArrayElement&) { return "array item"; }

template <typename Element>
std::string MakeBadTypeDescription(const Element& item, const char* context) {
  if (!item) {
    return std::string("item is missing for ") + context;
  }
  return MakeElementDescription(item) + " has type" + to_string(item.type()) +
         " (" +
         std::to_string(
             static_cast<std::underlying_type_t<bsoncxx::type>>(item.type())) +
         "), which is unsuitable for " + context;
}

}  // namespace
}  // namespace impl

BadType::BadType(const DocumentElement& item, const char* context)
    : MongoError(impl::MakeBadTypeDescription(item, context)) {}

BadType::BadType(const ArrayElement& item, const char* context)
    : MongoError(impl::MakeBadTypeDescription(item, context)) {}

bool IsOneOf(const DocumentElement& item, utils::Flags<ElementKind> kinds) {
  return impl::IsOneOf(item, kinds);
}

double ToDouble(const DocumentElement& item) { return impl::ToDouble(item); }

int64_t ToInt64(const DocumentElement& item) { return impl::ToInt64(item); }

std::chrono::system_clock::time_point ToTimePoint(const DocumentElement& item) {
  return impl::ToTimePoint(item);
}

bsoncxx::oid ToOid(const DocumentElement& item) { return impl::ToOid(item); }

std::string ToString(const DocumentElement& item) {
  return impl::ToString(item);
}

bool ToBool(const DocumentElement& item) { return impl::ToBool(item); }

bsoncxx::array::view ToArray(const DocumentElement& item) {
  return impl::ToArray(item);
}

bsoncxx::document::view ToDocument(const DocumentElement& item) {
  return impl::ToDocument(item);
}

std::vector<std::string> ToStringArray(const DocumentElement& array_item) {
  return impl::ToStringArray(array_item);
}

bool IsOneOf(const ArrayElement& item, utils::Flags<ElementKind> kinds) {
  return impl::IsOneOf(item, kinds);
}

double ToDouble(const ArrayElement& item) { return impl::ToDouble(item); }

int64_t ToInt64(const ArrayElement& item) { return impl::ToInt64(item); }

std::chrono::system_clock::time_point ToTimePoint(const ArrayElement& item) {
  return impl::ToTimePoint(item);
}

bsoncxx::oid ToOid(const ArrayElement& item) { return impl::ToOid(item); }

std::string ToString(const ArrayElement& item) { return impl::ToString(item); }

bool ToBool(const ArrayElement& item) { return impl::ToBool(item); }

bsoncxx::array::view ToArray(const ArrayElement& item) {
  return impl::ToArray(item);
}

bsoncxx::document::view ToDocument(const ArrayElement& item) {
  return impl::ToDocument(item);
}

std::vector<std::string> ToStringArray(const ArrayElement& array_item) {
  return impl::ToStringArray(array_item);
}

DocumentValue Or(const std::vector<bsoncxx::array::view_or_value>& docs) {
  bsoncxx::builder::basic::array builder;
  for (const auto& doc : docs) {
    builder.append(doc);
  }
  return bsoncxx::builder::basic::make_document(
      bsoncxx::builder::basic::kvp("$or", builder.extract()));
}

}  // namespace mongo
}  // namespace storages
