import collections
import contextlib
import dataclasses
import os
from typing import Dict
from typing import Generator
from typing import List
from typing import NoReturn
from typing import Optional
from typing import Union

from chaotic import error
from chaotic.front import types


@dataclasses.dataclass(frozen=True)
class ParserConfig:
    erase_prefix: str


@dataclasses.dataclass
class ParserState:
    # Current location in file
    infile_path: str

    schemas: Dict[str, types.Schema]
    schema_type: str = 'jsonschema'


class ParserError(error.BaseError):
    pass


class SchemaParser:
    def __init__(
            self,
            *,
            config: ParserConfig,
            full_filepath: str,
            full_vfilepath: str,
    ) -> None:
        self._config = config
        # Full filepath on real filesystem
        self.full_filepath: str = full_filepath
        # Full filepath on virtual filesystem, used by $ref
        self.full_vfilepath: str = full_vfilepath
        self._state = ParserState(infile_path='', schemas=dict())

    def parse_schema(self, infile_path: str, input__: dict) -> None:
        self._state.infile_path = ''

        with self._path_enter(infile_path) as _:
            data = self._parse_schema(input__)

            infile_path = self._state.infile_path
            # STUPID draft-handrews-json-schema-01 logic
            erase_prefix = self._config.erase_prefix
            if infile_path.startswith(erase_prefix):
                infile_path = infile_path[len(erase_prefix) :]

            path = self.full_vfilepath + '#' + infile_path
            if path in self._state.schemas:
                self._raise(f'Duplicate path: {path}')
            self._state.schemas[path] = data

    def _parse_schema(self, input__: dict) -> Union[types.Schema, types.Ref]:
        data = self.do_parse_schema(input__)
        source_location = types.SourceLocation(
            filepath=self.full_vfilepath, location=self._state.infile_path,
        )
        # pylint: disable=protected-access
        data._source_location = source_location  # type: ignore
        return data

    def do_parse_schema(self, input__: dict) -> Union[types.Schema, types.Ref]:
        if 'type' in input__:
            return self._parse_type(input__['type'], input__)
        elif '$ref' in input__:
            return self._parse_ref(input__['$ref'], input__)
        elif 'allOf' in input__:
            return self._parse_allof(input__['allOf'], input__)
        elif 'oneOf' in input__:
            return self._parse_oneof(input__['oneOf'], input__)
        else:
            self._raise('"type" is missing')

    def _parse_allof(self, variants: list, input__: dict) -> types.AllOf:
        raw = types.AllOfRaw(**input__)

        variables: List[types.Schema] = []
        with self._path_enter('allOf') as _:
            for i, variant in enumerate(variants):
                with self._path_enter(str(i)) as _:
                    type_ = self._parse_schema(variant)
                    if not isinstance(type_, (types.SchemaObject, types.Ref)):
                        self._raise(f'Non-object type in allOf: {type_.type}')  # type: ignore
                    variables.append(type_)
        obj = types.AllOf(allOf=variables)
        obj.x_properties = raw.x_properties  # type: ignore
        return obj

    def _parse_oneof(self, variants: list, input__: dict) -> types.Schema:
        raw = types.OneOfRaw(**input__)

        variables = []
        discriminator = input__.get('discriminator')
        with self._path_enter('oneOf') as _:
            if not discriminator:
                # oneOf w/o discriminator
                for i, variant in enumerate(variants):
                    with self._path_enter(str(i)) as _:
                        type_ = self._parse_schema(variant)
                        variables.append(type_)
                obj = types.OneOfWithoutDiscriminator(oneOf=variables)
                obj.x_properties = raw.x_properties  # type:ignore
                return obj

        return self._parse_oneof_w_discriminator(variants, input__)

    def _parse_oneof_i(
            self, variant: dict, discriminator_property: str,
    ) -> types.Ref:
        type_ = self._parse_schema(variant)
        if not isinstance(type_, types.Ref):
            self._raise('Not a $ref in oneOf with discriminator')

        assert self._state
        dest_type = self._state.schemas.get(type_.ref)
        # TODO: fix in TAXICOMMON-8910
        #
        # if not dest_type:
        #     self._raise(
        #         f'$ref to unknown type {type_.ref}, known types:\n'
        #         + '\n'.join(f' - {name}' for name in self._state.schemas),
        #     )
        if dest_type:
            if not isinstance(dest_type, types.SchemaObject):
                self._raise('oneOf $ref to non-object')
            if discriminator_property not in (dest_type.properties or {}):
                self._raise(
                    f'No discriminator property "{discriminator_property}"',
                )

        return type_

    def _parse_oneof_disc_mapping(
            self, user_mapping: dict, variables: List[types.Ref],
    ) -> List[List[str]]:
        idx_mapping = collections.defaultdict(list)
        with self._path_enter('discriminator/mapping') as _:
            for key, value in user_mapping.items():
                with self._path_enter(key) as _:
                    if not isinstance(value, str):
                        self._raise('Not a string in mapping')
                    abs_ref = self._make_abs_ref(value)
                    idx_mapping[abs_ref].append(key)

            mapping = []
            for ref in variables:
                assert isinstance(ref, types.Ref)
                abs_refs = idx_mapping.pop(ref.ref, None)
                if not abs_refs:
                    self._raise(f'Missing $ref in mapping: {ref.ref}')
                mapping.append(abs_refs)
            if idx_mapping:
                self._raise(
                    f'$ref(s) outside of oneOf: {list(idx_mapping.keys())}',
                )
        return mapping

    def _parse_oneof_w_discriminator(
            self, variants: list, input_: dict,
    ) -> types.OneOfWithDiscriminator:
        with self._path_enter('discriminator') as _:
            types.OneOfDiscriminatorRaw(**input_['discriminator'])

        discriminator_property = input_['discriminator']['propertyName']

        variables: List[types.Ref] = []
        with self._path_enter('oneOf') as _:
            for i, variant in enumerate(variants):
                with self._path_enter(str(i)) as _:
                    type_ = self._parse_oneof_i(
                        variant, discriminator_property,
                    )
                    variables.append(type_)

        user_mapping = input_['discriminator'].get('mapping')
        if user_mapping is not None:
            mapping = self._parse_oneof_disc_mapping(user_mapping, variables)
        else:
            mapping = [[ref.ref.split('/')[-1]] for ref in variables]
        obj = types.OneOfWithDiscriminator(
            oneOf=variables,
            discriminator_property=discriminator_property,
            mapping=mapping,
        )

        del input_['discriminator']
        del input_['oneOf']
        obj.x_properties = input_

        return obj

    @contextlib.contextmanager
    def _path_enter(self, path_component: str) -> Generator[None, None, None]:
        assert self._state
        old = self._state.infile_path
        if self._state.infile_path:
            self._state.infile_path += '/'
        self._state.infile_path += path_component
        # print(f'enter => {self._state.infile_path}')

        try:
            yield
        except types.FieldError as exc:
            assert self._state
            self._state.infile_path += '/' + exc.field
            self._raise(exc.msg)
        except ParserError:
            raise
        except Exception as exc:  # pylint: disable=broad-exception-caught
            self._raise(exc.__repr__())
        else:
            self._state.infile_path = old

    def _raise(self, msg: str) -> NoReturn:
        assert self._state
        raise ParserError(
            self.full_filepath,
            self._state.infile_path,
            self._state.schema_type,
            msg,
        )

    def _parse_type(self, type_: str, input_: dict) -> types.Schema:
        parser = TYPE_PARSERS.get(type_)
        if not parser:
            with self._path_enter('type') as _:
                self._raise(f'Unknown type "{type_}"')
            # Never reach
            return types.Schema()
        else:
            data = parser(self, input_)
            return data

    def _parse_array(self, input_: dict) -> types.Array:
        with self._path_enter('items') as _:
            input_ = input_.copy()
            if 'items' not in input_:
                self._raise('"items" is missing')

            items = self._parse_schema(input_['items'])
            del input_['items']
            return types.Array(items=items, **input_)

    def _parse_object(self, input_: dict) -> types.SchemaObject:
        fake = types.SchemaObjectRaw(**input_)

        with self._path_enter('properties') as _:
            new_props = {}
            if fake.properties:
                for prop in fake.properties:
                    with self._path_enter(prop) as _:
                        value = self._parse_schema(fake.properties[prop])
                        new_props[prop] = value

        add_props: bool | types.Schema | types.Ref
        with self._path_enter('additionalProperties') as _:
            if isinstance(fake.additionalProperties, bool):
                add_props = fake.additionalProperties
            else:
                add_props = self._parse_schema(fake.additionalProperties)

        obj = types.SchemaObject(
            additionalProperties=add_props,
            required=fake.required,
            properties=new_props,
        )
        obj.x_properties = fake.x_properties  # type: ignore
        return obj

    def _parse_boolean(self, input_: dict) -> types.Boolean:
        return types.Boolean(**input_)

    def _parse_int(self, input_: dict) -> types.Integer:
        format_str = input_.pop('format', None)

        fmt: Optional[types.IntegerFormat]
        if format_str:
            fmt = types.IntegerFormat.from_string(format_str)
        else:
            fmt = None
        return types.Integer(**input_, format=fmt)

    def _parse_number(self, input_: dict) -> types.Number:
        number = types.Number(**input_)
        if number.format not in [None, 'float', 'double']:
            self._raise(f'Unknown "format" ({number.format})')
        return number

    def _parse_string(self, input_: dict) -> types.String:
        format_str = input_.pop('format', None)
        fmt: Optional[types.StringFormat]
        if format_str:
            fmt = types.StringFormat.from_string(format_str)
        else:
            fmt = None
        return types.String(**input_, format=fmt)

    def _make_abs_ref(self, ref: str) -> str:
        assert self._state

        if ref.startswith('#'):
            # Local $ref
            return self.full_vfilepath + ref
        else:
            my_ref = '/'.join(self.full_vfilepath.split('/')[:-1])
            file, infile = ref.split('#')
            out_file = os.path.join(my_ref, file)
            # print(f'ref: {out_file} # {infile}')
            return out_file + '#' + infile

    def _parse_ref(self, ref: str, input_: dict) -> types.Ref:
        assert self._state

        fields = set(input_.keys())
        fields.remove('$ref')
        if 'description' in fields:
            # description is explicitly allowed in $ref
            fields.remove('description')

        if 'x-usrv-cpp-indirect' in fields:
            indirect = True
            fields.remove('x-usrv-cpp-indirect')
        elif 'x-taxi-cpp-indirect' in fields:
            indirect = True
            fields.remove('x-taxi-cpp-indirect')
        else:
            indirect = False

        fields.discard('x-usrv-cpp-name')
        fields.discard('x-taxi-cpp-name')

        if fields:
            self._raise(f'Unknown field(s) {list(fields)}')

        with self._path_enter('$ref') as _:
            abs_ref = self._make_abs_ref(ref)
            ref_value = types.Ref(
                ref=abs_ref, indirect=indirect, self_ref=False,
            )

            del input_['$ref']
            ref_value.x_properties = input_

            return ref_value

    def parsed_schemas(self) -> types.ParsedSchemas:
        assert self._state
        return types.ParsedSchemas(self._state.schemas)


# pylint: disable=protected-access
TYPE_PARSERS = {
    'boolean': SchemaParser._parse_boolean,
    'integer': SchemaParser._parse_int,
    'number': SchemaParser._parse_number,
    'string': SchemaParser._parse_string,
    'array': SchemaParser._parse_array,
    'object': SchemaParser._parse_object,
}
