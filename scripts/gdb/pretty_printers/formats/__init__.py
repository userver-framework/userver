import pretty_printers.formats.json as formats_json
import pretty_printers.formats.yaml as formats_yaml


def register_printers(pp_collection):
    formats_json.register_printers(pp_collection)
    formats_yaml.register_printers(pp_collection)
