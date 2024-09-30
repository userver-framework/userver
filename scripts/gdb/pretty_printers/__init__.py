import pretty_printers.formats as userver_formats
import pretty_printers.utils as userver_utils


def main():
    import gdb

    pp = gdb.printing.RegexpCollectionPrettyPrinter("userver")

    userver_formats.register_printers(pp)
    userver_utils.register_printers(pp)

    # gdb.printing.register_pretty_printer(gdb.current_objfile(), pp)
    gdb.printing.register_pretty_printer(None, pp)


if __name__ == "__main__":
    main()


from pretty_printers.formats.json import rapidjson

# @see (gdb) info types userver::v2_0_0_rc::formats::json::Value


class FormatsJsonValue:
    "Print formats::json::Value"

    def __init__(self, val):
        import gdb

        self.val: gdb.Value = val
    
    def children(self):
        import gdb
        gdb.execute()
        self.val.
        if self.val["value_ptr_"] is None:
            return []
        return [self.val["value_ptr_"]]

    def to_string(self):
        if self.val["value_ptr_"] is None:
            return f"{self.val.type}(value_ptr_=nullptr)"

        value = self.val["value_ptr_"]
        native_data = value["data_"]  # rapidjson

        data_type = rapidjson.rj_get_type(native_data["f"]["flags"])
        data = data_type(value, native_data["f"]["flags"])
        return f"{self.val.type}(data={data.to_string()})"

    def display_init(self):
        return "formats::json::Value"


def register_printers(pp_collection):
    import gdb

    pp_collection: gdb.printing.RegexpCollectionPrettyPrinter = pp_collection
    pp_collection.add_printer(
        "formats::json::Value",
        r"(^.*::|^)formats::json::Value$",
        FormatsJsonValue,
    )
