from pretty_printers.utils import fast_pimpl
from pretty_printers.formats.yaml import yaml_node

# @see (gdb) info types userver::v2_0_0_rc::formats::yaml::Value


class FormatsYamlValue:
    "Print formats::yaml::Value"

    def __init__(self, val):
        import gdb

        self.__val: gdb.Value = val
        self.__helper = fast_pimpl.UtilsFastPimpl(self.__val["value_pimpl_"])

    def to_string(self):
        return f"{self.__val.type}(data={self.__helper.underlying_value})"

    def display_init(self):
        return "formats::yaml::Value"


def register_printers(pp_collection):
    import gdb

    pp_collection: gdb.printing.RegexpCollectionPrettyPrinter = pp_collection
    pp_collection.add_printer(
        "formats::yaml::Value",
        r"(^.*::|^)formats::yaml::Value$",
        FormatsYamlValue,
    )
    pp_collection.add_printer(
        "YAML::Node",
        r"^YAML::Node$",
        yaml_node.YAMLNode,
    )
