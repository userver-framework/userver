class Ref:
    def __init__(self, ref: str) -> None:
        self._ref = ref

        parts = ref.split('#')
        if len(parts) != 2:
            raise Exception(f'Error in $ref ({ref}): there should be exactly one "#" inside')

        self.file = parts[0]
        self.fragment = parts[1]
