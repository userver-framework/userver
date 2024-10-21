from dataclasses import dataclass

import gdb


@dataclass
class Constants:
    # @see RAPIDJSON_UINT64_C2 at rapidjson/rapidjson.h
    RJ_UINT64_C2 = (0x0000FFFF << 32) | 0xFFFFFFFF

    # @see `info types` at rapidjson/document.h
    RJ_TALLOC = 'rapidjson::CrtAllocator'
    RJ_TENCODING = 'rapidjson::UTF8<char>'

    RJ_GENERIC_VALUE = f'rapidjson::GenericValue<{RJ_TENCODING}, {RJ_TALLOC}>'
    RJ_GENERIC_MEMBER = (
        f'rapidjson::GenericMember<{RJ_TENCODING}, {RJ_TALLOC}>'
    )

    RJ_GENERIC_OBJECT = f'rapidjson::GenericObject<true, {RJ_GENERIC_VALUE} >'

    # @see `enum Type` in rapidjson.h
    RJType_kNullType = 0  # //!<  null
    RJType_kFalseType = 1  # //!<  false
    RJType_kTrueType = 2  # //!<  true
    RJType_kObjectType = 3  # //!<  object
    RJType_kArrayType = 4  # //!<  array
    RJType_kStringType = 5  # //!<  string
    RJType_kNumberType = 6  # //!<  number

    # @see `enum` in rapidjson.h:
    RJFlag_kBoolFlag = 0x0008
    RJFlag_kNumberFlag = 0x0010
    RJFlag_kIntFlag = 0x0020
    RJFlag_kUintFlag = 0x0040
    RJFlag_kInt64Flag = 0x0080
    RJFlag_kUint64Flag = 0x0100
    RJFlag_kDoubleFlag = 0x0200
    RJFlag_kStringFlag = 0x0400
    RJFlag_kCopyFlag = 0x0800
    RJFlag_kInlineStrFlag = 0x1000

    # // Initial flags of different types.
    RJFlag_kNullFlag = RJType_kNullType
    # // These casts are added to suppress the warning on MSVC about
    # // bitwise operations between enums of different types.
    RJFlag_kTrueFlag = RJType_kTrueType | RJFlag_kBoolFlag
    RJFlag_kFalseFlag = RJType_kFalseType | RJFlag_kBoolFlag

    RJFlag_kNumberIntFlag = (
        RJType_kNumberType
        | RJFlag_kNumberFlag
        | RJFlag_kIntFlag
        | RJFlag_kInt64Flag
    )
    RJFlag_kNumberUintFlag = (
        RJType_kNumberType
        | RJFlag_kNumberFlag
        | RJFlag_kUintFlag
        | RJFlag_kUint64Flag
        | RJFlag_kInt64Flag
    )
    RJFlag_kNumberInt64Flag = (
        RJType_kNumberType | RJFlag_kNumberFlag | RJFlag_kInt64Flag
    )
    RJFlag_kNumberUint64Flag = (
        RJType_kNumberType | RJFlag_kNumberFlag | RJFlag_kUint64Flag
    )
    RJFlag_kNumberDoubleFlag = (
        RJType_kNumberType | RJFlag_kNumberFlag | RJFlag_kDoubleFlag
    )
    RJFlag_kNumberAnyFlag = (
        RJType_kNumberType
        | RJFlag_kNumberFlag
        | RJFlag_kIntFlag
        | RJFlag_kInt64Flag
        | RJFlag_kUintFlag
        | RJFlag_kUint64Flag
        | RJFlag_kDoubleFlag
    )

    RJFlag_kObjectFlag = RJType_kObjectType
    RJFlag_kArrayFlag = RJType_kArrayType
    RJFlag_kConstStringFlag = RJType_kStringType | RJFlag_kStringFlag
    RJFlag_kCopyStringFlag = (
        RJType_kStringType | RJFlag_kStringFlag | RJFlag_kCopyFlag
    )
    RJFlag_kShortStringFlag = (
        RJType_kStringType
        | RJFlag_kStringFlag
        | RJFlag_kCopyFlag
        | RJFlag_kInlineStrFlag
    )

    RJFlag_kTypeMask = 0x07


def rj_get_pointer(ptr, rj_type):
    # FIXME: support native pointer in case of w/o 48bit optimization
    # @see RAPIDJSON_48BITPOINTER_OPTIMIZATION,
    #      RAPIDJSON_GETPOINTER,
    #      RAPIDJSON_UINT64_C2 at rapidjson/rapidjson.h
    newptr = int(ptr) & Constants.RJ_UINT64_C2
    # just check rj_type is known or throw
    gdb.lookup_type(rj_type)
    return gdb.parse_and_eval(f'({rj_type}*){newptr}\n')


class RJBaseType:
    def __init__(self, val: gdb.Value, flags: gdb.Value):
        self.val = val
        self.flags = flags


class RJObjectType(RJBaseType):
    def __init__(self, val, flags):
        super().__init__(val, flags)
        data = val['data_']['o']
        self.size = int(data['size'])
        self.members = rj_get_pointer(
            data['members'], Constants.RJ_GENERIC_MEMBER,
        )
        if self.size:
            self.children = self.children_impl

    def children_impl(self):
        for i in range(self.size):
            member = self.members[i]
            name, value = member['name'], member['value']
            yield (f'[{i * 2}]', name)
            yield (f'[{i * 2 + 1}]', value)

    def display_hint(self):
        return 'map'

    def to_string(self):
        return f'object of size {self.size}'


class RJArrayType(RJBaseType):
    def __init__(self, val, flags):
        super().__init__(val, flags)
        data = self.val['data_']['a']
        self.size = int(data['size'])
        self.elements = rj_get_pointer(
            data['elements'], Constants.RJ_GENERIC_VALUE,
        )

    def children(self):
        for i in range(self.size):
            yield (f'[{i}]', self.elements[i])

    def display_hint(self):
        return 'array'

    def to_string(self):
        return f'array of size {self.size}'


class RJNumberType(RJBaseType):
    def _is(self, flag):
        return (self.flags & flag) == flag

    def to_string(self):
        data = self.val['data_']['n']
        if self._is(Constants.RJFlag_kNumberIntFlag):
            res = data['i']['i']
        elif self._is(Constants.RJFlag_kNumberUintFlag):
            res = data['u']['u']
        elif self._is(Constants.RJFlag_kNumberInt64Flag):
            res = data['i64']
        elif self._is(Constants.RJFlag_kNumberUint64Flag):
            res = data['u64']
        elif self._is(Constants.RJFlag_kNumberDoubleFlag):
            res = data['d']
        else:
            res = data
        return str(res)


class RJStringType(RJBaseType):
    def display_hint(self):
        return 'string'

    def to_string(self):
        data = self.val['data_']
        if (self.flags & Constants.RJFlag_kShortStringFlag) != 0:
            # FIXME: support other architectures
            # @see definition of LenPos in rapidjson/document.h
            return data['ss']['str'].string()
        return data['s']['str'].string()


class RJBoolType(RJBaseType):
    def to_string(self):
        if (self.flags & Constants.RJFlag_kBoolFlag) == 0:
            return '<bad-bool>'
        if self.flags == Constants.RJFlag_kFalseFlag:
            return 'false'
        if self.flags == Constants.RJFlag_kTrueFlag:
            return 'true'
        return str(self.val)


class RJNullType(RJBaseType):
    def to_string(self):
        return 'null'


def rj_get_type(flags):
    if flags == Constants.RJFlag_kArrayFlag:
        return RJArrayType
    if flags == Constants.RJFlag_kObjectFlag:
        return RJObjectType
    if (flags & Constants.RJFlag_kNumberFlag) == Constants.RJFlag_kNumberFlag:
        return RJNumberType
    if (flags & Constants.RJFlag_kStringFlag) == Constants.RJFlag_kStringFlag:
        return RJStringType
    if (flags & Constants.RJFlag_kBoolFlag) == Constants.RJFlag_kBoolFlag:
        return RJBoolType
    if flags == Constants.RJFlag_kNullFlag:
        return RJNullType
    raise Exception(f'Unsupported rapidjson flag assigning to type: {flags}')


class RapidJsonValue:
    "Print rapidjson::Value"

    def __init__(self, val: gdb.Value):
        flags = val['data_']['f']['flags']
        data_type = rj_get_type(flags)
        self.data = data_type(val, flags)
        if hasattr(self.data, 'to_string'):
            self.to_string = self.data.to_string
        if hasattr(self.data, 'display_hint'):
            self.display_hint = self.data.display_hint
        if hasattr(self.data, 'children'):
            self.children = self.data.children


class FormatsJsonValue:
    "Print formats::json::Value"

    def __init__(self, val: gdb.Value):
        self.value = val['value_ptr_']

    def to_string(self):
        return 'formats::json::Value'

    def children(self):
        yield ('value', self.value.dereference() if self.value else None)


def register_printers(
    pp_collection: gdb.printing.RegexpCollectionPrettyPrinter,
):
    pp_collection.add_printer(
        'formats::json::Value',
        r'(^.*::|^)formats::json::Value$',
        FormatsJsonValue,
    )
    pp_collection.add_printer(
        'formats::json::impl::Value',
        r'(^.*::|^)rapidjson::GenericValue<.*>$',
        RapidJsonValue,
    )


if __name__ == '__main__':
    pp = gdb.printing.RegexpCollectionPrettyPrinter('userver.formats.json')
    register_printers(pp)
    gdb.printing.register_pretty_printer(gdb.current_objfile(), pp)
