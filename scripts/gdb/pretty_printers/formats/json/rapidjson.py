from pretty_printers.formats.json import rapidjson_constants as constants


def rj_get_pointer(ptr, rj_type):
    import gdb

    # FIXME: support native pointer in case of w/o 48bit optimization
    # @see RAPIDJSON_48BITPOINTER_OPTIMIZATION,
    #      RAPIDJSON_GETPOINTER,
    #      RAPIDJSON_UINT64_C2 at rapidjson/rapidjson.h
    newptr = int(ptr) & constants.RJ_UINT64_C2
    # just check rj_type is known or throw
    gdb.lookup_type(rj_type)
    return gdb.parse_and_eval(f"({rj_type}*){newptr}\n")


class RJBaseType:
    def __init__(self, val, flags):
        self._val = val
        self._flags = flags


class RJObjectType(RJBaseType):
    def to_string(self):
        data = self._val["data_"]["o"]
        sz = int(data["size"])
        if sz == 0:
            return "{}"

        res = "{"
        sep = ""
        members = rj_get_pointer(
            data["members"],
            constants.RJ_GENERIC_MEMBER,
        )
        for n in range(sz):
            member = members[n]
            name, value = member["name"], member["value"]

            name_flags = name["data_"]["f"]["flags"]
            value_flags = value["data_"]["f"]["flags"]
            tname = rj_get_type(name_flags)
            tvalue = rj_get_type(value_flags)

            pname = tname(name, name_flags)
            pvalue = tvalue(value, value_flags)

            res += f"{sep}{pname.to_string()}:{pvalue.to_string()}"
            sep = ","
        res += "}"
        return res


class RJArrayType(RJBaseType):
    def to_string(self):
        data = self._val["data_"]["a"]
        sz = int(data["size"])
        if data["size"] == 0:
            return "[]"

        elements = rj_get_pointer(
            data["elements"],
            constants.RJ_GENERIC_VALUE,
        )
        res = "["
        sep = ""
        for n in range(sz):
            elem = elements[n]
            value_flags = elem["data_"]["f"]["flags"]
            tvalue = rj_get_type(value_flags)
            pvalue = tvalue(elem, value_flags)
            res += f"{sep}{pvalue.to_string()}"
            sep = ","
        res += "]"
        return res


class RJNumberType(RJBaseType):
    def _is(self, flag):
        return (self._flags & flag) == flag

    def to_string(self):
        data = self._val["data_"]["n"]
        if self._is(constants.RJFlag_kNumberIntFlag):
            return data["i"]["i"]
        if self._is(constants.RJFlag_kNumberUintFlag):
            return data["u"]["u"]
        if self._is(constants.RJFlag_kNumberInt64Flag):
            return data["i64"]
        if self._is(constants.RJFlag_kNumberUint64Flag):
            return data["u64"]
        if self._is(constants.RJFlag_kNumberDoubleFlag):
            return data["d"]
        return data


class RJStringType(RJBaseType):
    def to_string(self):
        data = self._val["data_"]
        if (self._flags & constants.RJFlag_kShortStringFlag) != 0:
            # FIXME: support other architectures
            # @see definition of LenPos in rapidjson/document.h
            return '"{0}"'.format(data["ss"]["str"].string())
        return '"{0}"'.format(data["s"]["str"].string())


class RJBoolType(RJBaseType):
    def to_string(self):
        if (self._flags & constants.RJFlag_kBoolFlag) == 0:
            return "<bad-bool>"
        if self._flags == constants.RJFlag_kFalseFlag:
            return "false"
        if self._flags == constants.RJFlag_kTrueFlag:
            return "true"
        return self._val


class RJNullType(RJBaseType):
    def to_string(self):
        return "null"


def rj_get_type(flags):
    if flags == constants.RJFlag_kArrayFlag:
        return RJArrayType
    if flags == constants.RJFlag_kObjectFlag:
        return RJObjectType

    if (flags & constants.RJFlag_kNumberFlag) == constants.RJFlag_kNumberFlag:
        return RJNumberType
    if (flags & constants.RJFlag_kStringFlag) == constants.RJFlag_kStringFlag:
        return RJStringType
    if (flags & constants.RJFlag_kBoolFlag) == constants.RJFlag_kBoolFlag:
        return RJBoolType
    if flags == constants.RJFlag_kNullFlag:
        return RJNullType

    raise Exception(
        f"Unsupported rapidjson flag assigning to type: {flags}",
    )
