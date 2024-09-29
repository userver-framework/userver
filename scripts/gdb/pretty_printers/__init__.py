import pretty_printers.formats as userver_formats
import pretty_printers.utils as userver_utils


def main():
    import gdb

    pp = gdb.printing.RegexpCollectionPrettyPrinter("userver")

    userver_formats.register_printers(pp)
    userver_utils.register_printers(pp)

    gdb.printing.register_pretty_printer(gdb.current_objfile(), pp)


if __name__ == "__main__":
    main()
