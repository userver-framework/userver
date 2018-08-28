#include <storages/mongo/mongo.hpp>

namespace storages {
namespace mongo {

BadType::BadType(const ::mongo::BSONElement& item, const char* context)
    : MongoError(std::string("item ") +
                 (item.ok()
                      ? (std::string("'") + item.fieldName() + "' has type " +
                         ::mongo::typeName(item.type()) + " (" +
                         std::to_string(item.type()) + "), which is unsuitable")
                      : "was not set") +
                 " for " + context) {}

bool OneOf(const ::mongo::BSONElement& item, utils::Flags<ElementKind> kinds) {
  if (!item.ok()) return !!(kinds & ElementKind::kMissing);

  switch (item.type()) {
    case ::mongo::BSONType::jstNULL:
      return !!(kinds & ElementKind::kNull);

    case ::mongo::BSONType::Bool:
      return !!(kinds & ElementKind::kBool);

    case ::mongo::BSONType::NumberInt:
    case ::mongo::BSONType::NumberDouble:
    case ::mongo::BSONType::NumberLong:
      return !!(kinds & ElementKind::kNumber);

    case ::mongo::BSONType::String:
    case ::mongo::BSONType::BinData:
      return !!(kinds & ElementKind::kString);

    case ::mongo::BSONType::Date:
      return !!(kinds & ElementKind::kTimestamp);

    case ::mongo::BSONType::Array:
      return !!(kinds & ElementKind::kArray);

    case ::mongo::BSONType::Object:
      return !!(kinds & ElementKind::kDocument);

    case ::mongo::BSONType::jstOID:
      return (kinds & ElementKind::kOid) || (kinds & ElementKind::kString);

    default:;
  }
  return false;
}

double ToDouble(const ::mongo::BSONElement& item) {
  if (OneOf(item, {ElementKind::kMissing, ElementKind::kNull})) return 0.0;
  if (OneOf(item, ElementKind::kBool)) return item.Bool() ? 1.0 : 0.0;
  ::mongo::BSONType type = item.type();
  if (type == ::mongo::BSONType::NumberInt) return item.Int();
  if (type == ::mongo::BSONType::NumberLong) return item.Long();
  if (type == ::mongo::BSONType::NumberDouble) return item.Double();
  if (type == ::mongo::BSONType::Date) return item.Date().asInt64();
  throw BadType(item, "ToDouble");
}

uint64_t ToUint(const ::mongo::BSONElement& item) {
  auto value = ToInt(item);
  if (value < 0) {
    throw std::runtime_error(std::string("value of '") + item.fieldName() +
                             "' (" + item.toString() +
                             ") cannot be converted to UINT");
  } else {
    return static_cast<uint64_t>(value);
  }
}

int64_t ToInt(const ::mongo::BSONElement& item) {
  if (OneOf(item, {ElementKind::kMissing, ElementKind::kNull})) return 0;
  if (OneOf(item, ElementKind::kBool)) return item.Bool() ? 1 : 0;

  ::mongo::BSONType type = item.type();
  if (type == ::mongo::BSONType::NumberInt) return item.Int();
  if (type == ::mongo::BSONType::NumberLong) return item.Long();
  if (type == ::mongo::BSONType::NumberDouble) return item.Double();
  if (type == ::mongo::BSONType::Date) return item.Date().asInt64();
  throw BadType(item, "ToInt");
}

std::chrono::system_clock::time_point ToTimePoint(
    const ::mongo::BSONElement& item) {
  using Clock = std::chrono::system_clock;

  if (!item.ok() || item.type() != ::mongo::BSONType::Date)
    throw BadType(item, "ToTimePoint");

  std::chrono::milliseconds timestamp{item.Date().asInt64()};
  if (timestamp > Clock::duration::max()) return Clock::time_point::max();
  if (timestamp < Clock::duration::min()) return Clock::time_point::min();
  return Clock::time_point(timestamp);
}

::mongo::OID ToOid(const ::mongo::BSONElement& item) {
  if (!item.ok() || item.type() != ::mongo::BSONType::jstOID)
    throw BadType(item, "ToOid");
  return item.OID();
}

std::string ToString(const ::mongo::BSONElement& item) {
  if (OneOf(item, {ElementKind::kMissing, ElementKind::kNull})) return {};
  if (OneOf(item, ElementKind::kBool)) return item.Bool() ? "true" : "";

  ::mongo::BSONType type = item.type();
  if (type == ::mongo::BSONType::NumberDouble)
    return std::to_string(ToDouble(item));
  if (OneOf(item, ElementKind::kNumber)) return std::to_string(ToInt(item));
  if (OneOf(item, ElementKind::kTimestamp))
    return std::to_string(
        std::chrono::system_clock::to_time_t(ToTimePoint(item)));
  if (type == ::mongo::BSONType::String) return item.String();
  if (type == ::mongo::BSONType::jstOID) return item.OID().toString();
  if (type == ::mongo::BSONType::BinData) {
    int len;
    const char* bytes = item.binData(len);
    return std::string(bytes, len);
  }
  throw BadType(item, "ToString");
}

bool ToBool(const ::mongo::BSONElement& item) {
  if (OneOf(item, {ElementKind::kMissing, ElementKind::kNull})) return false;
  if (OneOf(item, ElementKind::kBool)) return item.Bool();
  if (OneOf(item, ElementKind::kNumber)) return ToInt(item) != 0;
  if (OneOf(item, ElementKind::kTimestamp)) return true;
  if (OneOf(item, ElementKind::kString)) return !ToString(item).empty();
  if (OneOf(item, ElementKind::kArray)) return !item.Array().empty();
  if (OneOf(item, ElementKind::kDocument)) return !item.Obj().isEmpty();
  throw BadType(item, "ToBool");
}

std::vector<::mongo::BSONElement> ToArray(const ::mongo::BSONElement& item) {
  if (OneOf(item, {ElementKind::kMissing, ElementKind::kNull})) return {};
  if (OneOf(item, ElementKind::kArray)) return item.Array();
  throw BadType(item, "ToArray");
}

::mongo::BSONObj ToDocument(const ::mongo::BSONElement& item) {
  if (OneOf(item, {ElementKind::kMissing, ElementKind::kNull}))
    return kEmptyObject;
  if (OneOf(item, ElementKind::kDocument)) return item.Obj();
  throw BadType(item, "ToDocument");
}

std::vector<std::string> ToStringArray(const ::mongo::BSONElement& array_item) {
  std::vector<std::string> result;
  for (const auto& item : ToArray(array_item)) {
    result.push_back(ToString(item));
  }
  return result;
}

::mongo::Date_t Date(const std::chrono::system_clock::time_point& time_point) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             time_point.time_since_epoch())
      .count();
}

::mongo::BSONObj Or(const std::vector<::mongo::BSONObj>& docs) {
  ::mongo::BSONArrayBuilder builder;
  for (const auto& doc : docs) {
    builder.append(doc);
  }
  return BSON("$or" << builder.arr());
}

}  // namespace mongo
}  // namespace storages
