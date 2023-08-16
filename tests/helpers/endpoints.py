from utils import *

async def register_user(service_client, user):
    return await service_client.post(
        Routes.REGISTRATION,
        json=model_dump(user, include=RequiredFields.REGISTRATION.value)
    )

async def login_user(service_client, email, password):
    return await service_client.post(
        Routes.LOGIN,
        json=model_dump(user, include=RequiredFields.LOGIN.value)
    )