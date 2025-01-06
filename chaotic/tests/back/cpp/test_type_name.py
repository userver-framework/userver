from chaotic.back.cpp import type_name


def test_empty():
    name = type_name.TypeName('some_name')

    assert name.namespace() == ''
    assert name.in_local_scope() == 'some_name'
    assert name.in_global_scope() == 'some_name'
    assert name.in_scope('') == 'some_name'
    assert name.in_scope('some::ns') == 'some_name'
    assert name.in_scope('some_name') == 'some_name'


def test_ns():
    name = type_name.TypeName('ns::ns2::some_name')

    assert name.namespace() == 'ns::ns2'
    assert name.in_local_scope() == 'some_name'
    assert name.in_global_scope() == 'ns::ns2::some_name'
    assert name.in_scope('') == 'ns::ns2::some_name'
    assert name.in_scope('ns::some::ns2') == 'ns2::some_name'
    assert name.in_scope('ns::ns2::some') == 'some_name'
    assert name.in_scope('ns::ns2::some_name') == 'some_name'
    assert name.in_scope('ns::ns2::some_name::x') == 'some_name'


def test_modifiers():
    name = type_name.TypeName('ns::ns2::some_name')

    assert name.parent() == type_name.TypeName('ns::ns2')
    assert name.joinns('xxx') == type_name.TypeName('ns::ns2::some_name::xxx')
    assert name.add_suffix('xxx') == type_name.TypeName(
        'ns::ns2::some_namexxx',
    )
