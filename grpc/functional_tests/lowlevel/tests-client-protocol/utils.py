import grpc


def status_from_str(grpc_status_code_str: str) -> grpc.StatusCode:
    return getattr(grpc.StatusCode, grpc_status_code_str)


def status_to_str(grpc_status_code: grpc.StatusCode) -> str:
    return str(grpc_status_code.value[0])
