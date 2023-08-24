#include <userver/utils/statistics/percentile_format_json.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

std::string GetPercentileFieldName(double perc) {
  auto field_name = 'p' + std::to_string(perc);
  auto point_pos = field_name.find('.');
  if (point_pos != std::string::npos) {
    UASSERT(field_name.size() > point_pos + 1);
    if (field_name[point_pos + 1] != '0') {
      field_name[point_pos] = '_';
      field_name.resize(point_pos + 1 + 1);  // leave 1 digit
    } else {
      field_name.resize(point_pos);
    }
  }
  return field_name;
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
