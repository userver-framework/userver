import re

import gdb

# @see (gdb) info types -q utils::FastPimpl


class UtilsFastPimpl:
    "Print utils::FastPimpl<T>"

    RE_NUMERICAL = re.compile(r'(\d+)ul')

    def __init__(self, val: gdb.Value):
        self.__val = val
        self.__class_ommit = False

        # NOTE: this is a hack to normalize type representaion in tests because of different compilers
        # is producing different visualization of a numerical template argument, e.g. `32ul` vs `32`
        self.__valtype = re.sub(UtilsFastPimpl.RE_NUMERICAL, r'\1', str(self.__val.type))

        storage = self.__val['storage_']
        storage_ptr = storage.cast(storage.type.pointer())

        underlying_type = self.__val.type.template_argument(0)
        self.__underlying_ptr = storage_ptr.reinterpret_cast(underlying_type.pointer())

    def set_class_ommit(self, value):
        self.__class_ommit = value

    def unwrap(self):
        if self.__underlying_ptr:
            return self.__underlying_ptr.dereference()
        return None

    def _field(self, field):
        if self.__class_ommit:
            return f'{field}'
        return f'{self.__valtype}::{field}'

    def children(self):
        if not self.__underlying_ptr:
            yield (self._field('storage_'), 'Failed: look to the details using `disable pretty-printer global userver`')
        else:
            yield (self._field('storage_'), self.__underlying_ptr.dereference())


def utils_fast_pimpl_register_printers(pp_collection):
    pp_collection.add_printer(
        'utils::FastPimpl<T>',
        r'^(.*::|)utils::FastPimpl<.*>$',
        UtilsFastPimpl,
    )
    return pp_collection


if __name__ == '__main__':
    assert hasattr(gdb, 'printing'), (
        "userver's gdb pretty-printer utils::FastPimpl<T> cannot be initialized"
        ' because of module gdb.printing does not found'
    )
    gdb.printing.register_pretty_printer(
        gdb.current_objfile(),
        utils_fast_pimpl_register_printers(gdb.printing.RegexpCollectionPrettyPrinter('userver.utils')),
    )
