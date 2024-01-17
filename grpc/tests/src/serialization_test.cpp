#include <userver/ugrpc/proto_json.hpp>

#include <userver/utest/parameter_names.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
constexpr std::string_view kEmpty = R"()";
constexpr std::string_view kInt = R"(12)";
constexpr std::string_view kDouble = R"(12.123)";
constexpr std::string_view kBool = R"(false)";
constexpr std::string_view kString = R"("blah-blah-blah")";
constexpr std::string_view kSimpleObject = R"(
{"field1":123,"field2":1.23,"field3":"abacabad"}
)";
constexpr std::string_view kNestedObject = R"(
{"field1":{"field1":345,"field2":3.456},"field2":{},"field3":"abacaba"}
)";
constexpr std::string_view kObjectWithArray = R"(
{"field1":[1,2,3,true,"FaLsE"]}
)";
constexpr std::string_view kObjectWithNestedArray = R"(
{"field1":[["abacaba",4,5,6],2,3,true,"FaLsE"]}
)";
constexpr std::string_view kObjectWithObjectWithArray = R"(
{"field1":{"field1":[1,2,3,"false"]},"field2":"abaCaba"}
)";
constexpr std::string_view kEmptyArray = R"([])";
constexpr std::string_view kSimpleArray = R"(
[true, false, "blah", 1243, 123.321]
)";
constexpr std::string_view kObjectsArray = R"(
[{"field1":123,"field2":1.23,"field3":"abacaba"},
{"field1":489,"field2":4.23,"field3":"abacaba2"}]
)";
constexpr std::string_view kNestedArray = R"(
[[1,2,3,false,true],[],[3,4,5,"aba","caba"],[]]
)";
constexpr std::string_view kNestedArrayOfObjects = R"(
[[],[1,{"aba":123,"caba":false},false],
[{"aba":123,"caba":[1,1.1,true],"daba":{"aba":"caba"}}],[]]
)";
constexpr std::string_view kNestedNestedArray = R"(
[[[1,false,true],[1,false,true]],[],[[1,false,true],[1,false,true],
{"field1":123,"field2":false,"field3":true}],
[{"field1":[1,false,true,25.56],"field2":false,"field3":true},
[1,false,true],{"field1":123,"field2":false,"field3":[1,false,true,25.56]}]]
)";
constexpr std::string_view kRandom1 = R"(
[430432054.89273405,false,-1729588561.6063952,
[{"correct":1921625716,"hurt":[true,[false,[344852348.8847656,[false,
[true,"suggest",{"bottom":{"anyone":false,"clay":false,
"gun":"orbit","hardly":1728677734,"public":918187319.5833945,"tea":"scared"},
"construction":1412974965,"front":true,"prize":-1913771482,
"public":1017557609.0607877,"whether":"road"},false,true,true],
1447611265.204443,false,"now","solve"],true,false,"famous",1498336369],
true,1573964445.510466,-1737527993,47745880.630069256],
-602633450.4563785,true,1796198824.705258,"give"],"own":"present",
"smoke":true,"this":false,"wheat":"anyone"},"tightly",
1769816023.5432687,2140279846,"charge",-152924466],"union","does"]
)";
constexpr std::string_view kRandom2 = R"(
{"border":1720446263,"boy":-1350556571,"exclaimed":true,
"leave":[{"dear":{"almost":true,"everybody":true,
"grown":false,"hall":false,"middle":"poor","shout":
{"among":["to",{"against":1740687013.9191742,"at":"at",
"clothes":[[[1538950668,true,"dust","grade",true,true],
-2035008309.8318982,false,"floor",{"cap":"himself",
"early":"owner","own":"offer","oxygen":false,"pride":
-1966584589.4698162,"rise":150708630},false],-1691993923.457893,
"habit",-1925581036,true,true],"hold":-169594077.32530355,
"round":1110484028,"war":true},-237807713,"observe",-775512415,-780439803.8418674],
"battle":737377501,"both":"frog","powerful":"warn","skin":true,
"slight":-39832290.47358084}},"know":false,"recent":-1920960342.2496037,
"sharp":"form","softly":false,"write":false},10247500.801391125,
-134568968,true,"chief","pressure"],"my":462154457,"nest":106930535.56662798}
)";
constexpr std::string_view kRandom3 = R"(
[{"cannot":"bring","chamber":[[{"camp":"born","creature":-1902377750.185803,
"drive":"play","grass":false,"research":{"cell":false,"colony":
1412158193.952931,"mill":[64558505,true,354303063.59697485,["bright",
{"break":[false,true,"point",true,-1282596705.9991045,-780715057],
"draw":"handle","flies":"blank","path":false,"plant":false,"though":"earn"},
"silver",1755711518,false,false],1907759588.761578,"sale"],"other":1191178829,
"whatever":2110094181,"zero":2029006959},"stairs":true},-1427737613.0292025,
true,-518593868,-2000451860.4843307,true],"ear",2134274955.953373,
false,false,true],"necessary":"tropical","negative":"spread",
"sharp":false,"tall":false},"valuable",false,302765324.49991226,false,"deep"]
)";
constexpr std::string_view kRandom4 = R"(
[{"appearance":1821127394.9021797,"avoid":-2136622017,"cap":false,
"double":false,"laid":false,"mostly":"length","poem":true,
"rhyme":-1655541012.3937745,"town":false,"yourself":"evening"},
{"beneath":"brief","its":583470167.3982387,"judge":"practical",
"poor":false,"pull":[{"ants":-745957571.1039724,"die":-1652209275.7126856,
"equal":"smallest","final":-1995981894.8087854,"general":false,
"had":"rich","jar":-1197244504.3537493,"organization":-943767711.1343532,
"village":true,"whatever":{"advice":"theory","alone":true,"atom":"wait",
"chart":-389409297,"driven":true,"dust":"knowledge","exercise":"nodded",
"frozen":{"activity":488296329,"development":-1064678971,"dozen":1016131007,
"fifteen":false,"flight":false,"knew":[-1641945149.919229,true,"repeat"
,false,1768450891,"southern",{"after":false,"aloud":1411891803,
"announced":true,"border":"would","forgot":[1850399147.8201718,
true,{"breath":"soap","buffalo":"origin","drove":"slept","faster":"steady",
"lips":"large","nature":"appropriate","organized":-828589361,
"shallow":269577649,"touch":-1255965389,"usually":-1830294075.2045627},
false,-1053184311,"fly",false,-1713275431.5087686,
-488395463.6128659,true],"fourth":-1901113943.8041863,"island":
"figure","principle":"daughter","raw":false,"tide":"level"},
803569739.5550513,"main",1702317161],"mix":false,"operation":true,
"part":"disappear","run":"cabin"},"home":"yellow","upward":"farther"}},
402157061.3341732,"package",1377860589.8480487,-1026442458.8432412,"vote",
"connected","metal","community","easy"],"riding":"wish",
"sets":true,"talk":"flies","tomorrow":-983073546,"warm":false},
318362813,1559189536,true,1048458332.8802476,
false,true,"line",-1812281629.0818615]
)";

struct Param {
  explicit Param(std::string test_name, std::string_view json)
      : test_name(test_name) {
    if (json.empty()) {
      to_cast = formats::json::Value();
    } else {
      to_cast = formats::json::FromString(json);
    }
  }

  std::string test_name;
  formats::json::Value to_cast;
};

const std::vector<Param> TestParams() {
  return {
      Param("empty", kEmpty),
      Param("int", kInt),
      Param("double", kDouble),
      Param("bool", kBool),
      Param("string", kString),
      Param("simple_object", kSimpleObject),
      Param("nested_object", kNestedObject),
      Param("object_with_array", kObjectWithArray),
      Param("object_with_nested_array", kObjectWithNestedArray),
      Param("object_with_object_with_array", kObjectWithObjectWithArray),
      Param("empty_array", kEmptyArray),
      Param("simple_array", kSimpleArray),
      Param("objects_array", kObjectsArray),
      Param("nested_array", kNestedArray),
      Param("nested_array_of_objects", kNestedArrayOfObjects),
      Param("nested_nested_array", kNestedNestedArray),
      Param("random1", kRandom1),
      Param("random2", kRandom2),
      Param("random3", kRandom3),
      Param("random4", kRandom4),
  };
}

}  // namespace

class SerializationTest : public testing::TestWithParam<Param> {};

TEST_P(SerializationTest, JsonTest) {
  auto param = GetParam();

  auto proto_struct = formats::parse::Parse(
      param.to_cast, formats::parse::To<google::protobuf::Value>{});
  auto result = formats::serialize::Serialize(
      proto_struct, formats::serialize::To<formats::json::Value>{});
  EXPECT_EQ(param.to_cast, result);
}

INSTANTIATE_TEST_SUITE_P(
    /*no prefix*/, SerializationTest, testing::ValuesIn(TestParams()),
    utest::PrintTestName());

USERVER_NAMESPACE_END
