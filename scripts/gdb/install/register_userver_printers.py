import os
import sys

import gdb

sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import userver_printers  # noqa: E402


def userver_register_printers(objfile):
    pp = gdb.printing.RegexpCollectionPrettyPrinter('userver')
    userver_printers.userver_register_printers(pp)
    gdb.printing.register_pretty_printer(objfile, pp)


userver_register_printers(gdb.current_objfile())
