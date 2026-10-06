import dataclasses
import sys

import gdb
import gdb.printing

# @see (gdb) info types -q formats::yaml::Value


def formats_yaml_require_visualizer(value):
    visualizer = gdb.default_visualizer(value)
    if visualizer is None:
        raise RuntimeError(
            f'userver YAML pretty-printer requires a GDB pretty-printer for {value.type}; '
            'load the matching libstdc++ or libc++ pretty-printers first'
        )
    return visualizer


def formats_yaml_visualizer_to_string(value):
    rendered = formats_yaml_require_visualizer(value).to_string()
    if isinstance(rendered, gdb.Value):
        return rendered.string()
    try:
        lazy_value = rendered.value()
        lazy_length = rendered.length
    except AttributeError:
        return str(rendered).strip('"')
    if lazy_length >= 0:
        return lazy_value.string(length=lazy_length)
    return lazy_value.string()


class YAMLNodeUndefined:
    def __init__(self, val):
        self.__val = val

    def children(self):
        yield (f'{self.__val.type}', 'undefined')


class YAMLNodeNull:
    def __init__(self, val):
        pass

    def to_string(self):
        return f'(null)'


class YAMLNodeScalar:
    def __init__(self, val):
        self.__val = val
        self.__val_unq = formats_yaml_visualizer_to_string(self.__val['m_scalar'])

    def _is_number(self) -> bool:
        try:
            float(self.__val_unq)
            return True
        except ValueError:
            return False

    def _is_boolean(self) -> bool:
        return self.__val_unq in ['true', 'false', 'yes', 'no']

    def to_string(self) -> str:
        if self._is_number() or self._is_boolean():
            return self.__val_unq
        return self.__val['m_scalar']


class YAMLNodeSequence:
    def __init__(self, val):
        self.__val = val
        self.__vis = formats_yaml_require_visualizer(self.__val['m_sequence'])
        if next(iter(self.__vis.children()), None) is None:
            self.to_string = lambda: '[]'
        else:
            self.children = self._children

    def _children(self):
        for i, value in self.__vis.children():
            yield (f'[{i}]', value.dereference())

    def display_hint(self):
        return 'array'


class YAMLNodeMap:
    def __init__(self, val):
        self.__val = val
        self.__vis = formats_yaml_require_visualizer(self.__val['m_map'])
        first_child = next(iter(self.__vis.children()), None)
        if first_child is None:
            self.to_string = lambda: '{}'
        else:
            _, first_value = first_child
            try:
                first_value['first']
                first_value['second']
            except (gdb.error, TypeError):
                self.__children_are_pairs = False
            else:
                self.__children_are_pairs = True
            self.children = self._children

    def _children(self):
        if self.__children_are_pairs:
            for pos, value in self.__vis.children():
                yield (f'{pos}-key', value['first'].dereference())
                yield (f'{pos}-value', value['second'].dereference())
        else:
            for pos, value in self.__vis.children():
                yield (pos, value.dereference())

    def display_hint(self):
        return 'map'


@dataclasses.dataclass
class FormatsYamlContants:
    # @see yaml-cpp/node/type.h Yaml::NodeType::value
    YAML_NodeType_Undefined = 0
    YAML_NodeType_Null = 1
    YAML_NodeType_Scalar = 2
    YAML_NodeType_Sequence = 3
    YAML_NodeType_Map = 4

    @staticmethod
    def yamlcpp_get_type(data_type):
        if data_type == FormatsYamlContants.YAML_NodeType_Map:
            return YAMLNodeMap

        if data_type == FormatsYamlContants.YAML_NodeType_Sequence:
            return YAMLNodeSequence

        if data_type == FormatsYamlContants.YAML_NodeType_Scalar:
            return YAMLNodeScalar

        if data_type == FormatsYamlContants.YAML_NodeType_Null:
            return YAMLNodeNull

        if data_type == FormatsYamlContants.YAML_NodeType_Undefined:
            return YAMLNodeUndefined

        raise Exception(f'Unsupported yaml-cpp assigning to type: {data_type}')


class YAMLDetailNode(gdb.Value):
    "Print YAML::detail::node"

    def __init__(self, val):
        self.__val = val

        node_refvis = iter(formats_yaml_require_visualizer(self.__val['m_pRef']).children())
        node_ref_child = next(node_refvis, None)
        if node_ref_child is None:
            self.children = self._node_pref_nullptr
            return
        node_ref = node_ref_child[1]

        if not node_ref['m_pData']:
            self.children = self._node_pdata_nullptr
            return

        datavis = formats_yaml_require_visualizer(node_ref['m_pData'])
        self.data = next(iter(datavis.children()))[1]
        self.node_printer = FormatsYamlContants.yamlcpp_get_type(self.data['m_type'])(self.data.dereference())
        if hasattr(self.node_printer, 'to_string'):
            self.to_string = self._node_to_string
        if hasattr(self.node_printer, 'children'):
            self.children = self._node_children
        if hasattr(self.node_printer, 'display_hint'):
            self.display_hint = self._node_display_hint

    def _node_pref_nullptr(self):
        yield (f'{self.__val.type}::m_pRef', 'nullptr')

    def _node_pdata_nullptr(self):
        yield (f'{self.__val.type}::m_pData', 'nullptr')

    def _node_null(self):
        return f'{self.__val.type}(null)'

    def _node_to_string(self):
        return self.node_printer.to_string()

    def _node_display_hint(self):
        return self.node_printer.display_hint()

    def _node_children(self):
        for child_tuple in self.node_printer.children():
            yield child_tuple


class YAMLNode:
    "Print YAML::Node"

    def __init__(self, val: gdb.Value):
        self.__val = val
        self.__class_ommit = False
        if not self.__val['m_isValid']:
            self.to_string = self._node_invalid
            return
        if not self.__val['m_pNode']:
            self.to_string = self._node_null
            return

        self._node = self.__val['m_pNode'].dereference()

        try:
            gdb.lookup_type('YAML::detail::node')
            gdb.lookup_type('YAML::detail::node_data')
            self.children = self._children
        except Exception:
            sys.stderr.write('Incomplete type YAML::detail::node. Check yaml-cpp were built with -g flag.\n')
            self.children = self._node_child_incomplete_type

    def set_class_ommit(self, value):
        self.__class_ommit = value

    def _field(self, field):
        if self.__class_ommit:
            return f'{field}'
        return f'{self.__val.type}::{field}'

    def _node_invalid(self):
        return f'{self.__val.type}(isValid=false,invalidKey={self.__val["m_invalidKey"]})'

    def _node_null(self):
        return f'{self.__val.type}(null)'

    def _node_child_incomplete_type(self):
        yield (f'{self.__val.type}', f'Incomplete {self._node.type}')

    def _children(self):
        yield (self._field('m_pNode'), self._node)


class FormatsYamlValue:
    "Print formats::yaml::Value"

    def __init__(self, val: gdb.Value):
        self.__val = val
        self.__vis = gdb.default_visualizer(self.__val['value_pimpl_'])
        while self.__vis and hasattr(self.__vis, 'unwrap'):
            self.__vis = gdb.default_visualizer(self.__vis.unwrap())
        if not self.__vis:
            self.children = self._children_default
        else:
            if hasattr(self.__vis, 'set_class_ommit'):
                self.__vis.set_class_ommit(True)
            if hasattr(self.__vis, 'to_string'):
                self.display_hint = lambda: 'string'
                self.to_string = lambda: self.__vis.to_string()
            if hasattr(self.__vis, 'children'):
                self.display_hint = lambda: 'array'
                self.__vis.display_hint = lambda: 'array'
                self.children = lambda: self.__vis.children()
            if hasattr(self.__vis, 'display_hint'):
                self.display_hint = lambda: self.__vis.display_hint()

    def _to_string(self):
        return self.__vis.to_string()

    def _children_default(self):
        yield ('value_pimpl_', self.__val['value_pimpl_'])

    def _children(self):
        for key, value in self.__vis.children():
            yield key, value


def formats_yaml_register_printers(pp_collection):
    pp_collection.add_printer(
        'formats::yaml::Value',
        r'(^.*::|^)formats::yaml::Value$',
        FormatsYamlValue,
    )
    pp_collection.add_printer(
        'YAML::detail::node',
        r'^YAML::detail::node$',
        YAMLDetailNode,
    )
    pp_collection.add_printer(
        'YAML::Node',
        r'^YAML::Node$',
        YAMLNode,
    )
    return pp_collection


if __name__ == '__main__':
    assert hasattr(gdb, 'printing'), (
        "userver's gdb pretty-printer formats::yaml::Value cannot be initialized"
        ' because of module gdb.printing does not found'
    )
    gdb.printing.register_pretty_printer(
        gdb.current_objfile(),
        formats_yaml_register_printers(gdb.printing.RegexpCollectionPrettyPrinter('userver.formats.yaml')),
    )
