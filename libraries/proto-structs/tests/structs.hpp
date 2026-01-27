#pragma once

#include <userver/proto-structs/io/fwd.hpp>
#include <userver/proto-structs/io/supported_types.hpp>
#include <userver/proto-structs/type_mapping.hpp>

#include "struct_simple.hpp"

namespace messages {
class ConversionFailure;
class Scalar;
class WellKnownStd;
class WellKnownUsrv;
class WellKnownJson;
class Optional;
class Repeated;
class Map;
class Oneof;
class Indirect;
class Strong;
class Erroneous;
}  // namespace messages

namespace structs {

enum class ConversionFailureType { kUnspecified = 0, kException = 1, kError = 2, kErrorWithUnknownField = 3 };

struct ConversionFailure {
    ConversionFailureType error_type = {};
};

}  // namespace structs

namespace proto_structs::traits {

template <>
struct CompatibleMessageTrait<::structs::ConversionFailure> {
    using Type = ::messages::ConversionFailure;
};

}  // namespace proto_structs::traits

namespace structs {

enum class TestEnum { kUnspecified = 0, kValue1 = 1, kValue2 = 2 };

struct Scalar {
    using ProtobufMessage = messages::Scalar;

    bool f1 = {};
    int32_t f2 = {};
    uint32_t f3 = {};
    int64_t f4 = {};
    uint64_t f5 = {};
    float f6 = {};
    double f7 = {};
    std::string f8 = {};
    std::string f9 = {};
    TestEnum f10 = {};
    std::size_t f11 = {};
};

struct WellKnownStd {
    using ProtobufMessage = messages::WellKnownStd;

    std::chrono::time_point<std::chrono::system_clock> f1 = {};
    std::chrono::milliseconds f2 = {};
    std::chrono::year_month_day f3 = {};
    std::chrono::hh_mm_ss<std::chrono::microseconds> f4 = {};
};

struct WellKnownUsrv {
    using ProtobufMessage = messages::WellKnownUsrv;

    ups::Any f1 = {};
    ups::Timestamp f2 = {};
    ups::Duration f3 = {};
    ups::Date f4 = {};
    ups::TimeOfDay f5 = {};
    USERVER_NAMESPACE::utils::datetime::TimeOfDay<std::chrono::microseconds> f6 = {};
    USERVER_NAMESPACE::decimal64::Decimal<3> f7 = {};
};

struct WellKnownJson {
    using ProtobufMessage = messages::WellKnownJson;

    formats::json::Value f1 = {};
    formats::json::Array f2 = {};
    formats::json::Object f3 = {};
};

struct Optional {
    using ProtobufMessage = messages::Optional;

    std::optional<int32_t> f1 = {};
    std::optional<std::string> f2 = {};
    std::optional<TestEnum> f3 = {};
    std::optional<Simple> f4 = {};
};

struct Repeated {
    using ProtobufMessage = messages::Repeated;

    std::vector<int32_t> f1 = {};
    std::vector<std::string> f2 = {};
    std::vector<TestEnum> f3 = {};
    std::vector<Simple> f4 = {};
};

struct Map {
    using ProtobufMessage = messages::Map;

    std::map<int32_t, int32_t> f1 = {};
    std::unordered_map<std::string, std::string> f2 = {};
    std::map<bool, TestEnum> f3 = {};
    std::unordered_map<std::string, Simple, USERVER_NAMESPACE::utils::StrCaseHash> f4 = {};
};

struct Oneof {
    using ProtobufMessage = messages::Oneof;
    using Type = ups::Oneof<int32_t, std::string, TestEnum, Simple>;

    Type test_oneof = {};
};

struct Indirect {
    using ProtobufMessage = messages::Indirect;

    template <typename T>
    using Box = USERVER_NAMESPACE::utils::Box<T>;

    using OneofType = ups::Oneof<Box<Simple>, Box<std::string>>;

    Box<Simple> f1 = {};
    std::optional<Box<ups::Duration>> f2 = {};
    std::vector<Box<Simple>> f3 = {};
    std::map<int32_t, Box<Simple>> f4 = {};
    OneofType test_oneof = {};
    Box<int32_t> f7 = {};
    Box<std::vector<Box<TestEnum>>> f8 = {};
    Box<std::map<std::string, Box<Simple>>> f9 = {};
};

struct Strong {
    struct Tag {};

    using ProtobufMessage = messages::Strong;
    using F1Strong = USERVER_NAMESPACE::utils::StrongTypedef<Tag, int32_t>;
    using F2Strong = USERVER_NAMESPACE::utils::StrongTypedef<Tag, std::string>;
    using F3Strong = USERVER_NAMESPACE::utils::StrongTypedef<Tag, TestEnum>;
    using F4Strong = USERVER_NAMESPACE::utils::StrongTypedef<Tag, Simple>;
    using F5Strong = USERVER_NAMESPACE::utils::StrongTypedef<Tag, ups::Duration>;
    using OneofType = ups::Oneof<F5Strong>;

    F1Strong f1 = {};
    std::optional<F2Strong> f2 = {};
    std::vector<F3Strong> f3 = {};
    std::map<int32_t, F4Strong> f4 = {};
    OneofType test_oneof = {};
};

struct Erroneous {
    using ProtobufMessage = messages::Erroneous;
    using OneofType = ups::Oneof<ConversionFailure>;

    std::optional<ConversionFailure> f1 = {};
    std::vector<ConversionFailure> f2 = {};
    std::unordered_map<int32_t, ConversionFailure> f3 = {};
    OneofType test_oneof = {};
};

ConversionFailure
ReadProtoStruct(ups::io::ReadContext&, ups::io::To<ConversionFailure>, const messages::ConversionFailure&);
void WriteProtoStruct(ups::io::WriteContext& ctx, const ConversionFailure&, messages::ConversionFailure&);

Scalar ReadProtoStruct(ups::io::ReadContext&, ups::io::To<Scalar>, const messages::Scalar&);
void WriteProtoStruct(ups::io::WriteContext&, const Scalar&, messages::Scalar&);
void WriteProtoStruct(ups::io::WriteContext&, Scalar&&, messages::Scalar&);

WellKnownStd ReadProtoStruct(ups::io::ReadContext&, ups::io::To<WellKnownStd>, const messages::WellKnownStd&);
void WriteProtoStruct(ups::io::WriteContext&, const WellKnownStd&, messages::WellKnownStd&);
void WriteProtoStruct(ups::io::WriteContext&, WellKnownStd&&, messages::WellKnownStd&);

WellKnownUsrv ReadProtoStruct(ups::io::ReadContext&, ups::io::To<WellKnownUsrv>, const messages::WellKnownUsrv&);
void WriteProtoStruct(ups::io::WriteContext&, const WellKnownUsrv&, messages::WellKnownUsrv&);
void WriteProtoStruct(ups::io::WriteContext&, WellKnownUsrv&&, messages::WellKnownUsrv&);

WellKnownJson ReadProtoStruct(ups::io::ReadContext&, ups::io::To<WellKnownJson>, const messages::WellKnownJson&);
void WriteProtoStruct(ups::io::WriteContext&, const WellKnownJson&, messages::WellKnownJson&);
void WriteProtoStruct(ups::io::WriteContext&, WellKnownJson&&, messages::WellKnownJson&);

Optional ReadProtoStruct(ups::io::ReadContext&, ups::io::To<Optional>, const messages::Optional&);
void WriteProtoStruct(ups::io::WriteContext&, const Optional&, messages::Optional&);
void WriteProtoStruct(ups::io::WriteContext&, Optional&&, messages::Optional&);

Repeated ReadProtoStruct(ups::io::ReadContext&, ups::io::To<Repeated>, const messages::Repeated&);
void WriteProtoStruct(ups::io::WriteContext&, const Repeated&, messages::Repeated&);
void WriteProtoStruct(ups::io::WriteContext&, Repeated&&, messages::Repeated&);

Map ReadProtoStruct(ups::io::ReadContext&, ups::io::To<Map>, const messages::Map&);
void WriteProtoStruct(ups::io::WriteContext&, const Map&, messages::Map&);
void WriteProtoStruct(ups::io::WriteContext&, Map&&, messages::Map&);

Oneof ReadProtoStruct(ups::io::ReadContext&, ups::io::To<Oneof>, const messages::Oneof&);
void WriteProtoStruct(ups::io::WriteContext&, const Oneof&, messages::Oneof&);
void WriteProtoStruct(ups::io::WriteContext&, Oneof&&, messages::Oneof&);

Indirect ReadProtoStruct(ups::io::ReadContext&, ups::io::To<Indirect>, const messages::Indirect&);
void WriteProtoStruct(ups::io::WriteContext&, const Indirect&, messages::Indirect&);
void WriteProtoStruct(ups::io::WriteContext&, Indirect&&, messages::Indirect&);

Strong ReadProtoStruct(ups::io::ReadContext&, ups::io::To<Strong>, const messages::Strong&);
void WriteProtoStruct(ups::io::WriteContext&, const Strong&, messages::Strong&);
void WriteProtoStruct(ups::io::WriteContext&, Strong&&, messages::Strong&);

Erroneous ReadProtoStruct(ups::io::ReadContext&, ups::io::To<Erroneous>, const messages::Erroneous&);
void WriteProtoStruct(ups::io::WriteContext&, const Erroneous&, messages::Erroneous&);

void CheckScalarEqual(const Scalar&, const messages::Scalar&);
void CheckWellKnownStdEqual(const WellKnownStd&, const messages::WellKnownStd&);
void CheckWellKnownUsrvEqual(const WellKnownUsrv&, const messages::WellKnownUsrv&);
void CheckWellKnownJsonEqual(const WellKnownJson&, const messages::WellKnownJson&);
void CheckOptionalEqual(const Optional&, const messages::Optional&);
void CheckRepeatedEqual(const Repeated&, const messages::Repeated&);
void CheckMapEqual(const Map&, const messages::Map&);
void CheckOneofEqual(const Oneof&, const messages::Oneof&);
void CheckIndirectEqual(const Indirect&, const messages::Indirect&);
void CheckStrongEqual(const Strong&, const messages::Strong&);

}  // namespace structs
