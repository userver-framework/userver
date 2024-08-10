from typing import List
from typing import Union


class TypeName:
    def __init__(self, name: Union[str, List[str]]) -> None:
        if isinstance(name, str):
            self._components = name.split('::')
        else:
            self._components = name

    def __eq__(self, other: object) -> bool:
        if isinstance(other, TypeName):
            return self._components == other._components
        else:
            return False

    def __str__(self) -> str:
        return '::'.join(self._components)

    def add_suffix(self, suffix: str) -> 'TypeName':
        comp = self._components.copy()
        comp[-1] += suffix
        return TypeName(comp)

    def joinns(self, item: str) -> 'TypeName':
        comp = self._components.copy()
        comp.append(item)
        return TypeName(comp)

    def parent(self) -> 'TypeName':
        comp = self._components[:-1]
        return TypeName(comp)

    def in_local_scope(self) -> str:
        return self._components[-1]

    def relative_to(self, ns: str) -> 'TypeName':
        namespaces = ns.split('::')
        components = self._components
        while (
                namespaces
                and namespaces[0] == components[0]
                and len(components) > 1
        ):
            namespaces = namespaces[1:]
            components = components[1:]
        return TypeName('::'.join(components))

    def namespace(self) -> str:
        return '::'.join(self._components[:-1])

    def in_global_scope(self) -> str:
        return '::'.join(self._components)

    def in_scope(self, ns: str) -> str:
        return self.relative_to(ns).in_global_scope()
