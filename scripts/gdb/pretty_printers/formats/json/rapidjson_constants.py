# @see RAPIDJSON_UINT64_C2 at rapidjson/rapidjson.h
RJ_UINT64_C2 = (0x0000FFFF << 32) | 0xFFFFFFFF

# @see `info types` at rapidjson/document.h
RJ_TALLOC = 'rapidjson::CrtAllocator'
RJ_TENCODING = 'rapidjson::UTF8<char>'

RJ_GENERIC_VALUE = f'rapidjson::GenericValue<{RJ_TENCODING}, {RJ_TALLOC}>'
RJ_GENERIC_MEMBER = f'rapidjson::GenericMember<{RJ_TENCODING}, {RJ_TALLOC}>'

RJ_GENERIC_OBJECT = f'rapidjson::GenericObject<true, {RJ_GENERIC_VALUE} >'

# @see `enum Type` in rapidjson.h
RJType_kNullType    =  0  #  //!<  null
RJType_kFalseType   =  1  #  //!<  false
RJType_kTrueType    =  2  #  //!<  true
RJType_kObjectType  =  3  #  //!<  object
RJType_kArrayType   =  4  #  //!<  array
RJType_kStringType  =  5  #  //!<  string
RJType_kNumberType  =  6  #  //!<  number

# @see `enum` in rapidjson.h:
RJFlag_kBoolFlag       = 0x0008
RJFlag_kNumberFlag     = 0x0010
RJFlag_kIntFlag        = 0x0020
RJFlag_kUintFlag       = 0x0040
RJFlag_kInt64Flag      = 0x0080
RJFlag_kUint64Flag     = 0x0100
RJFlag_kDoubleFlag     = 0x0200
RJFlag_kStringFlag     = 0x0400
RJFlag_kCopyFlag       = 0x0800
RJFlag_kInlineStrFlag  = 0x1000

# // Initial flags of different types.
RJFlag_kNullFlag          =  RJType_kNullType
#// These casts are added to suppress the warning on MSVC about
#// bitwise operations between enums of different types.
RJFlag_kTrueFlag          =  RJType_kTrueType     |  RJFlag_kBoolFlag
RJFlag_kFalseFlag         =  RJType_kFalseType    |  RJFlag_kBoolFlag

RJFlag_kNumberIntFlag     =  (
    RJType_kNumberType   |  RJFlag_kNumberFlag   |
    RJFlag_kIntFlag      |  RJFlag_kInt64Flag
)
RJFlag_kNumberUintFlag    =  (
    RJType_kNumberType   |  RJFlag_kNumberFlag |
    RJFlag_kUintFlag     |  RJFlag_kUint64Flag |
    RJFlag_kInt64Flag
)
RJFlag_kNumberInt64Flag   =  (
    RJType_kNumberType   |  RJFlag_kNumberFlag   |
    RJFlag_kInt64Flag
)
RJFlag_kNumberUint64Flag  =  (
    RJType_kNumberType   |  RJFlag_kNumberFlag   |
    RJFlag_kUint64Flag
)
RJFlag_kNumberDoubleFlag  =  (
    RJType_kNumberType   |  RJFlag_kNumberFlag   |
    RJFlag_kDoubleFlag
)
RJFlag_kNumberAnyFlag     =  (
    RJType_kNumberType   |  RJFlag_kNumberFlag   |
    RJFlag_kIntFlag      |  RJFlag_kInt64Flag       |
    RJFlag_kUintFlag    |  RJFlag_kUint64Flag  |
    RJFlag_kDoubleFlag
)

RJFlag_kObjectFlag        =  RJType_kObjectType
RJFlag_kArrayFlag         =  RJType_kArrayType
RJFlag_kConstStringFlag   =  (
    RJType_kStringType   |  RJFlag_kStringFlag
)
RJFlag_kCopyStringFlag    =  (
    RJType_kStringType   |  RJFlag_kStringFlag   |
    RJFlag_kCopyFlag
)
RJFlag_kShortStringFlag   =  (
    RJType_kStringType   |  RJFlag_kStringFlag   |
    RJFlag_kCopyFlag     |  RJFlag_kInlineStrFlag
)

RJFlag_kTypeMask          =  0x07
