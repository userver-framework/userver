# @see (gdb) info types userver::v2_0_0_rc::utils::FastPimpl


class UtilsFastPimpl:
    "Print utils::FastPimpl<T>"

    def __init__(self, val):
        import gdb

        self.__val: gdb.Value = val

    @property
    def underlying_value(self):
        storage = self.__val["storage_"]
        storage_ptr = storage.cast(storage.type.pointer())

        underlying_type = self.__val.type.template_argument(0)
        return storage_ptr.reinterpret_cast(
            underlying_type.pointer(),
        ).dereference()

    def to_string(self):
        return f"{self.__val.type}(storage_={self.underlying_value})"

    def display_init(self):
        return "utils::FastPimpl<T>"


def register_printers(pp_collection):
    import gdb

    pp_collection: gdb.printing.RegexpCollectionPrettyPrinter = pp_collection
    pp_collection.add_printer(
        "utils::FastPimpl<T>",
        r"(^.*::|^)utils::FastPimpl<.*>$",
        UtilsFastPimpl,
    )
