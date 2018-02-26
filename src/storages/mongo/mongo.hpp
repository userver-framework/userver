#pragma once

#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include <mongo/bson/bson.h>
#include <boost/optional.hpp>

#include <utils/flags.hpp>

namespace storages {
namespace mongo {

static const ::mongo::BSONObj kEmptyObject;
static const ::mongo::BSONArray kEmptyArray;

enum class SortOrder { kDescending = -1, kAscending = 1 };

class MongoError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

class BadType : public MongoError {
  using MongoError::MongoError;

 public:
  BadType(const ::mongo::BSONElement& type, const char* context);
};

enum class ElementKind {
  kMissing = 1 << 0,
  kNull = 1 << 1,
  kBool = 1 << 2,
  kNumber = 1 << 3,
  kString = 1 << 4,
  kTimestamp = 1 << 5,
  kArray = 1 << 6,
  kDocument = 1 << 7,
  kOid = 1 << 8,
};

bool OneOf(const ::mongo::BSONElement&, utils::Flags<ElementKind>);

double ToDouble(const ::mongo::BSONElement&);
uint64_t ToUint(const ::mongo::BSONElement&);
int64_t ToInt(const ::mongo::BSONElement&);
std::chrono::system_clock::time_point ToTimePoint(const ::mongo::BSONElement&);
::mongo::OID ToOid(const ::mongo::BSONElement&);
std::string ToString(const ::mongo::BSONElement&);
bool ToBool(const ::mongo::BSONElement&);
std::vector<::mongo::BSONElement> ToArray(const ::mongo::BSONElement&);
::mongo::BSONObj ToDocument(const ::mongo::BSONElement&);
std::vector<std::string> ToStringArray(const ::mongo::BSONElement&);

namespace impl {

template <typename T>
struct ConversionTraits {};

template <>
struct ConversionTraits<double> {
  static double Convert(const ::mongo::BSONElement& item) {
    return ToDouble(item);
  }
};

template <>
struct ConversionTraits<bool> {
  static bool Convert(const ::mongo::BSONElement& item) { return ToBool(item); }
};

}  // namespace impl

template <typename T>
boost::optional<T> ToOptional(const ::mongo::BSONElement& item) {
  if (OneOf(item, {ElementKind::kMissing, ElementKind::kNull})) {
    return {};
  }
  return impl::ConversionTraits<T>::Convert(item);
}

::mongo::Date_t Date(const std::chrono::system_clock::time_point&);

::mongo::BSONObj Or(const std::vector<::mongo::BSONObj>& docs);

}  // namespace mongo
}  // namespace storages
