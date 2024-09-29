# Original took from /usr/share/gcc-13/python/libstdcxx/v6/printers.py


class std_vector_iterator(object):
    def __init__(self, start, finish, bitvec):
        self._bitvec = bitvec
        if bitvec:
            self._item = start["_M_p"]
            self._so = 0
            self._finish = finish["_M_p"]
            self._fo = finish["_M_offset"]
            itype = self._item.dereference().type
            self._isize = 8 * itype.sizeof
        else:
            self._item = start
            self._finish = finish
        self._count = 0

    def __iter__(self):
        return self

    def __next__(self):
        count = self._count
        self._count = self._count + 1
        if self._bitvec:
            if self._item == self._finish and self._so >= self._fo:
                raise StopIteration
            elt = bool(self._item.dereference() & (1 << self._so))
            self._so = self._so + 1
            if self._so >= self._isize:
                self._item = self._item + 1
                self._so = 0
            return (count, elt)
        else:
            if self._item == self._finish:
                raise StopIteration
            elt = self._item.dereference()
            self._item = self._item + 1
            return (count, elt)

    @staticmethod
    def from_vector(vec):
        import gdb

        is_bool = vec.type.template_argument(0).code == gdb.TYPE_CODE_BOOL
        return std_vector_iterator(
            vec["_M_impl"]["_M_start"], vec["_M_impl"]["_M_finish"], is_bool
        )
