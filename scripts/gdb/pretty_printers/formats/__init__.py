import userver_printers.formats.json.printers as formats_json
import userver_printers.formats.yaml.printers as formats_yaml


def formats_register_printers(pp_collection):
    formats_json.formats_json_register_printers(pp_collection)
    formats_yaml.formats_yaml_register_printers(pp_collection)
