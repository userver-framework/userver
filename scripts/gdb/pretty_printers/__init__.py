import userver_printers.formats as formats
import userver_printers.utils as utils


def userver_register_printers(pp_collection):
    formats.formats_register_printers(pp_collection)
    utils.utils_register_printers(pp_collection)
