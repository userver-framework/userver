"""Fields options"""

import dataclasses
from typing import Dict
from typing import Final


@dataclasses.dataclass(frozen=True)
class FieldOptions:
    """Codegen fields options"""

    #: Should wrap a field in utils::Box.
    should_box: Final[bool] = False


@dataclasses.dataclass(frozen=True)
class PluginOptions:
    field_options: Dict[str, FieldOptions]


_FIELDS_OPTIONS = {
    # ---------------TESTS------------------------
    'simple.Self.self': {'should_box': True},
    'simple.MyMap.self': {'should_box': True},
    'simple.Third.c': {'should_box': True},
    'simple.CycleStart.cycle': {'should_box': True},
    'simple.Main1.Inner.cycle': {'should_box': True},
    'simple.Main2.Inner.cycle': {'should_box': True},
    'simple.NewCycle.Inner2Below.InnerInner.inner': {'should_box': True},
    'simple.NewCycle2.Inner2Above.InnerInner.inner': {'should_box': True},
    # -----------------------------------------
    'google.protobuf.Struct.fields': {'should_box': True},
    'google.api.BackendRule.overrides_by_request_protocol': {'should_box': True},
    # api/proto/yandex/eats/shared/predicate/codegen-module.yaml
    'yandex.eats.shared.predicate.Predicate.init': {'should_box': True},
    'buf.validate.RepeatedRules.items': {'should_box': True},
    'buf.validate.MapRules.keys': {'should_box': True},
    'buf.validate.MapRules.values': {'should_box': True},
    # api/proto/yandex/taxi/examples/service/v1/codegen-module.yaml
    'yandex.taxi.examples.service.v1.NotCondition.condition': {'should_box': True},
    'yandex.eunomia.control.v1.Difference.base': {'should_box': True},
    'yandex.eunomia.control.v1.Difference.subtract': {'should_box': True},
    # taxi/schemas/grpc/ddd/business/quality-control
    'yandex.business.quality_control.v1.models.Entity.ExamState.FutureState.state': {'should_box': True},
}


def load_fields_options() -> PluginOptions:
    return PluginOptions(
        field_options={full_field_name: FieldOptions(**options) for full_field_name, options in _FIELDS_OPTIONS.items()}
    )
