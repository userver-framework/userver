import sys

from pretty_printers.helpers import std_vector
from pretty_printers.helpers import std_smart_ptr


# @see yaml-cpp/node/type.h Yaml::NodeType::value
YAML_NodeType_Undefined = 0
YAML_NodeType_Null = 1
YAML_NodeType_Scalar = 2
YAML_NodeType_Sequence = 3
YAML_NodeType_Map = 4


class YAMLDetailNode:
    "Print YAML::detail::node"

    def __init__(self, val):
        self.__val = val

    def to_string(self):
        node_ref = std_smart_ptr.dereference(self.__val['m_pRef'])
        if not node_ref:
            return 'YAML::detail::node(m_pRef=nullptr)'

        data = std_smart_ptr.dereference(node_ref['m_pData'])
        if not data:
            return 'YAML::detail::node(m_pData=nullptr)'

        data_type = data['m_type']
        if data_type == YAML_NodeType_Map:
            res = '{'
            sep = ''
            for pos, kv in std_vector.std_vector_iterator.from_vector(data['m_map']):
                first = kv['first'].dereference()
                second = kv['second'].dereference()
                res += f'{sep}{first}:{second}'
                sep = ','
            res += '}'
            return f'{res}'

        if data_type == YAML_NodeType_Sequence:
            res = '['
            sep = ''
            for pos, value in std_vector.std_vector_iterator.from_vector(data['m_sequence']):
                res += f'{sep}{value.dereference()}'
                sep = ','
            res += ']'
            return f'{res}'

        if data_type == YAML_NodeType_Scalar:
            return f'{data["m_scalar"]}'

        if data_type == YAML_NodeType_Null:
            return 'null'

        if data_type == YAML_NodeType_Undefined:
            return 'undefined'
        return f'{data}'


class YAMLNode:
    "Print YAML::Node"

    def __init__(self, val):
        self.__val = val

    def to_string(self):
        import gdb
        if not self.__val['m_isValid']:
            return f'{self.__val.type}(isValid=false,invalidKey={self.__val["m_invalidKey"]})'

        if not self.__val['m_pNode']:
            return f'{self.__val.type}(null)'

        node = self.__val['m_pNode'].dereference()
        try:
            gdb.lookup_type('YAML::detail::node')
        except Exception:
            sys.stderr.write(
                'Incomplete type YAML::detail::node. ' 'Check yaml-cpp were build with -g flag.\n'
            )
            return f'{self.__val.type}({node.type})'

        return f'{self.__val.type}({YAMLDetailNode(node).to_string()})'

    def display_init(self):
        return 'YAML::Node'
