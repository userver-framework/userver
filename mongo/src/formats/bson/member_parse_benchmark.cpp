#include <benchmark/benchmark.h>

#include <userver/formats/bson.hpp>
#include <userver/formats/bson/serialize.hpp>
#include <userver/formats/json.hpp>

#include <array>

// Test inspired by internal candidates service.
// Data was taken from the DB and personal data was removed.

USERVER_NAMESPACE_BEGIN

namespace {

using formats::parse::To;

constexpr std::size_t kBenchRows = 5;
const formats::bson::impl::BsonHolder bench_bson_data[kBenchRows] = {
    formats::bson::FromJsonString(
        R"({ "_id" : "999002_00611f99bab343948edbfb9a0ec7eb39", "real_clid" : "024bdc1317e14105bd64579b559ca69d", "name" : "******", "_state" : "absent", "phone" : "+7**********", "clid" : "999002", "driver_license" : "*********", "class" : [ "minivan" ], "park_boost" : 9.99, "supported_promotions" : [ ], "requirements" : { "nosmoking" : true, "bicycle" : true, "childbooster_amount" : 0, "animaltransport" : true, "childseat_amount" : 0, "conditioner" : true, "ski" : true, "infantseat_amount" : 0 }, "uuid" : "00611f99bab343948edbfb9a0ec7eb39", "car" : { "raw_model" : "Volkswagen Caddy", "model_code" : "CADDY", "color_code" : "200204", "allowed_classes" : [ "minivan" ], "age" : 2014, "number" : "*********", "color" : "коричневый", "allowed_tariffs" : [ "2" ], "mark_code" : "VOLKSWAGEN", "permit" : "*****", "model" : "Volkswagen Caddy", "model_boost" : {  }, "price" : 922183.3333333334 }, "real_name" : "Park-b-03", "score" : { "base_total" : 0.5773503, "total" : 0.5773503, "complete_daily" : [ ], "complete_today" : 0 }, "permit" : "*****", "tariffs" : [ "2" ], "status" : "free", "status_updated" : "2016-02-02T07:49:03.031Z", "free" : true, "hiring_details" : { "hiring_type" : "commercial", "hiring_date" : "2018-04-09T15:31:58.883Z" }, "updated_ts" : 1553506333 })")
        .GetBson(),

    formats::bson::FromJsonString(
        R"({ "_id" : "999002_006a313328ca483b9a458a1810a9ece6", "real_clid" : "3034f992bffb451f8c3eed50df96bde6", "name" : "******", "_state" : "absent", "phone" : "+7**********", "clid" : "999002", "driver_license" : "*********", "class" : [ "minivan" ], "park_boost" : 9.99, "supported_promotions" : [ ], "requirements" : { "nosmoking" : true, "bicycle" : true, "childbooster_amount" : 0, "animaltransport" : true, "universal" : true, "childseat_amount" : 0, "conditioner" : true, "check" : true, "infantseat_amount" : 0 }, "uuid" : "006a313328ca483b9a458a1810a9ece6", "car" : { "raw_model" : "SEAT Alhambra", "model_code" : "ALHAMBRA", "color_code" : "CACECB", "allowed_classes" : [ "business" ], "age" : 2013, "number" : "*********", "color" : "серебристый", "allowed_tariffs" : [ ], "mark_code" : "SEAT", "permit" : "*****", "model" : "Seat Alhambra", "model_boost" : {  }, "price" : 1725000 }, "real_name" : "Park-w-02", "score" : { "base_total" : 0.5773503, "total" : 0.5773503, "complete_daily" : [ ], "complete_today" : 0 }, "permit" : "*****", "tariffs" : [ "2" ], "hiring_details" : { "hiring_type" : "commercial", "hiring_date" : "2018-04-09T15:31:58.901Z" }, "updated_ts" : 1553506333 })")
        .GetBson(),

    formats::bson::FromJsonString(
        R"({ "_id" : "999002_006c47cfe282412bb9ee5bcabc566400", "real_clid" : "411e91e456b24c6683a530c2461bcd37", "name" : "******", "_state" : "absent", "phone" : "+7**********", "clid" : "999002", "driver_license" : "*********", "class" : [ "minivan" ], "park_boost" : 9.99, "supported_promotions" : [ ], "requirements" : { "nosmoking" : true, "childchair_min" : 6, "childbooster_amount" : 1, "universal" : true, "childchair_max" : 12, "childseat_amount" : 0, "check" : true, "infantseat_amount" : 0 }, "uuid" : "006c47cfe282412bb9ee5bcabc566400", "car" : { "raw_model" : "Mercedes Vito", "model_code" : "VITO", "allowed_classes" : [ "minivan" ], "age" : 2014, "number" : "*********", "color" : "каричевый", "allowed_tariffs" : [ "2" ], "mark_code" : "MERCEDES", "permit" : "*****", "model" : "Mercedes-Benz Vito", "model_boost" : {  }, "price" : 1787362.5 }, "real_name" : "Park-t-03", "score" : { "base_total" : 0.5773503, "total" : 0.5773503, "complete_daily" : [ ], "complete_today" : 0 }, "permit" : "*****", "tariffs" : [ "2" ], "status" : "free", "status_updated" : "2015-12-02T17:29:32.745Z", "free" : true, "hiring_details" : { "hiring_type" : "commercial", "hiring_date" : "2018-04-09T15:31:58.918Z" }, "updated_ts" : 1553506333 })")
        .GetBson(),

    formats::bson::FromJsonString(
        R"({ "_id" : "999002_006cc07271a04929b87e779a1168d804", "_state" : "absent", "phone" : "+7**********", "clid" : "999002", "driver_license" : "*********", "class" : [ "vip", "business" ], "park_boost" : 9.99, "supported_promotions" : [ ], "requirements" : { "nosmoking" : true, "bicycle" : true, "childbooster_amount" : 0, "animaltransport" : true, "universal" : true, "yellowcarnumber" : true, "childseat_amount" : 0, "conditioner" : true, "check" : true, "infantseat_amount" : 0 }, "uuid" : "006cc07271a04929b87e779a1168d804", "car" : { "raw_model" : "Hyundai Equus", "model_code" : "EQUUS", "color_code" : "CACECB", "allowed_classes" : [ "vip", "business" ], "age" : 2013, "number" : "*********", "color" : "серебристый", "allowed_tariffs" : [ "4", "3" ], "mark_code" : "HYUNDAI", "permit" : "*****", "model" : "Hyundai Equus", "model_boost" : {  }, "price" : 1682571.3333333333 }, "name" : "******", "score" : { "base_total" : 0.5773503, "total" : 0.5773503, "complete_daily" : [ ], "complete_today" : 0 }, "permit" : "*****", "tariffs" : [ "4", "3" ], "status" : "free", "status_updated" : "2015-12-21T07:50:33.706Z", "free" : true, "hiring_details" : { "hiring_type" : "commercial", "hiring_date" : "2018-04-09T15:31:58.935Z" }, "updated_ts" : 1553506333 })")
        .GetBson(),

    formats::bson::FromJsonString(
        R"({ "_id" : "999002_006e877430b9480fa0678c9284c2320a", "real_clid" : "a23954f769e249fda3938ee4f8bc40da", "requirements" : { "childchair_min" : 0.1, "childchair_max" : 12, "willsmoke" : true }, "uuid" : "006e877430b9480fa0678c9284c2320a", "car" : { "raw_model" : "Nissan Almera", "model_code" : "ALMERA", "color_code" : "FFC0CB", "allowed_classes" : [ "business" ], "age" : 2012, "number" : "*********", "color" : "розовый", "allowed_tariffs" : [ "1" ], "mark_code" : "NISSAN", "permit" : "*****", "model" : "Nissan Almera", "price" : 409838.3333333333 }, "_state" : "absent", "real_name" : "Park-q-02", "phone" : "+7**********", "clid" : "999002", "score" : { "base_total" : 0.5773503, "total" : 0.5773503, "complete_daily" : [ ], "complete_today" : 0 }, "driver_license" : "*********", "tariffs" : [ "1" ], "class" : [ "econom" ], "name" : "******", "permit" : "*****", "is_from_gold_park" : false, "mqc_passed" : "2014-11-07T13:01:44.548Z", "hiring_details" : { "hiring_type" : "commercial", "hiring_date" : "2018-04-09T15:31:58.952Z" }, "updated_ts" : 1553506333 })")
        .GetBson(),
};

namespace names {
static const std::string kId = "_id";
static const std::string kCar = "car";
static const std::string kClasses = "class";
static const std::string kUuid = "uuid";
static const std::string kLicense = "driver_license";
static const std::string kRequirements = "requirements";
static const std::string kState = "_state";
static const std::string kUpdated = "updated";
static const std::string kDbid = "db_id";
static const std::string kUpdatedTs = "updated_ts";
static const std::string kGrades = "grades";
static const std::string kGradeClass = "class";
static const std::string kGradeValue = "value";

namespace car {

static const std::string kNumber = "number";
static const std::string kModel = "model";
static const std::string kMarkCode = "mark_code";
static const std::string kAge = "age";
static const std::string kPrice = "price";

}  // namespace car

namespace requirements {

static const std::string kChildSeats = "childseats";

}  // namespace requirements
}  // namespace names

namespace models {

struct DriverId {
  std::string dbid;
  std::string uuid;
};

class Requirements {
 public:
  using ChildSeat = std::vector<short>;
  struct ChildSeats : std::vector<ChildSeat> {};  // Ugly! For testing only!

  using Value = std::variant<bool, short, ChildSeats>;

  Requirements() = default;

  void Add(const std::string& name, Value&& value) {
    values_[name] = std::move(value);
  }

 protected:
  std::unordered_map<std::string, Value> values_;
};

class ClassesGrade {
 public:
  using value_t = short;

  static constexpr std::size_t kMaxCount = 8;

  void Set(const std::string& name, const value_t value) {
    values_[std::hash<std::string>{}(name) % kMaxCount] = value;
  }

 private:
  std::array<value_t, kMaxCount + 1> values_{};
};

struct ProfileCar {
  std::string number;
  std::string model;
  std::string mark_code;
  short age = 0;
  uint32_t price = 0;
};

struct Profile {
  models::DriverId driver_id;
  std::string license;
  ProfileCar car;
  Requirements available_requirements;
  models::ClassesGrade grades;
};

models::DriverId Parse(const formats::bson::Value& val, To<models::DriverId>) {
  models::DriverId driver_id;
  driver_id.uuid = val[names::kUuid].As<std::string>();
  driver_id.dbid = val[names::kUuid].As<std::string>();  // changed
  return driver_id;
}

models::ProfileCar Parse(const formats::bson::Value& val,
                         To<models::ProfileCar>) {
  models::ProfileCar car;
  car.number = val[names::car::kNumber].As<std::string>();
  car.model = val[names::car::kModel].As<std::string>(std::string{});
  car.mark_code = val[names::car::kMarkCode].As<std::string>(std::string{});
  car.age = val[names::car::kAge].As<short>(0);
  car.price = val[names::car::kPrice].As<double>(0);
  return car;
}

models::Requirements::ChildSeats Parse(
    const formats::bson::Value& bson,
    formats::parse::To<models::Requirements::ChildSeats>) {
  if (!bson.IsArray()) return {};

  models::Requirements::ChildSeats seats;
  for (const auto& chair_supported_classes : bson) {
    if (!chair_supported_classes.IsArray()) return seats;

    models::Requirements::ChildSeat seat;
    for (const auto& chair_class : chair_supported_classes) {
      if (!chair_class.IsInt64()) return seats;
      seat.push_back(chair_class.As<short>());
    }

    std::sort(seat.begin(), seat.end());
    seats.push_back(std::move(seat));
  }

  return seats;
}

models::Requirements Parse(const formats::bson::Value& bson,
                           To<models::Requirements>) {
  models::Requirements result;

  for (auto it = bson.begin(); it != bson.end(); ++it) {
    const std::string& name = it.GetName();

    if (name == names::requirements::kChildSeats)
      result.Add(name, it->As<models::Requirements::ChildSeats>());
    else if (it->IsBool())
      result.Add(name, it->As<bool>());
    else if (it->IsInt64())
      result.Add(name, it->As<short>());
  }

  return result;
}

models::ClassesGrade Parse(const formats::bson::Value& bson,
                           To<models::ClassesGrade>) {
  bson.CheckArrayOrNull();
  models::ClassesGrade ret;
  for (const auto& el : bson) {
    const auto class_name = el[names::kGradeClass].As<std::string>();
    const auto value =
        el[names::kGradeValue].As<models::ClassesGrade::value_t>();
    ret.Set(class_name, value);
  }
  return ret;
}

models::Profile Parse(const formats::bson::Value& val, To<models::Profile>) {
  models::Profile profile;
  profile.driver_id = val.As<models::DriverId>();
  profile.car = val[names::kCar].As<models::ProfileCar>();
  profile.license = val[names::kLicense].As<std::string>();
  profile.available_requirements =
      val[names::kRequirements].As<models::Requirements>(
          models::Requirements{});
  profile.grades =
      val[names::kGrades].As<models::ClassesGrade>(models::ClassesGrade{});
  return profile;
}

}  // namespace models

}  // anonymous namespace

void bson_parse_full(benchmark::State& state) {
  static unsigned i = 0;

  for (auto _ : state) {
    auto bson = formats::bson::Document(bench_bson_data[++i % kBenchRows]);

    const auto res = bson.As<models::Profile>();
    benchmark::DoNotOptimize(res);
  }
}
BENCHMARK(bson_parse_full);

void bson_parse_access(benchmark::State& state) {
  static unsigned i = 0;

  for (auto _ : state) {
    state.PauseTiming();
    auto bson = formats::bson::Document(bench_bson_data[++i % kBenchRows]);
    bson[names::kLicense].As<std::string>();
    state.ResumeTiming();

    const auto res = bson.As<models::Profile>();
    benchmark::DoNotOptimize(res);
  }
}
BENCHMARK(bson_parse_access);

USERVER_NAMESPACE_END
