class BaseError(Exception):
    def __init__(
            self,
            full_filepath: str,
            infile_path: str,
            schema_type: str,
            msg: str,
    ) -> None:
        self.full_filepath = full_filepath
        self.infile_path = infile_path
        self.schema_type = schema_type
        self.msg = msg

    def __str__(self) -> str:
        return '\n'.join(
            [
                '',
                '===============================================================',
                f'Unhandled error while processing {self.full_filepath}',
                f'Path "{self.infile_path}", Format "{self.schema_type}"',
                'Error:',
                f'{self.msg}',
                '===============================================================',
            ],
        )
