from typing import Optional

from pydantic import BaseModel, ConfigDict, Field

from .oauth_flows import OAuthFlows


class SecurityScheme(BaseModel):
    """
    Defines a security scheme that can be used by the operations.
    Supported schemes are HTTP authentication,
    an API key (either as a header, a cookie parameter or as a query parameter),
    OAuth2's common flows (implicit, password, client credentials and authorization code)
    as defined in [RFC6749](https://tools.ietf.org/html/rfc6749),
    and [OpenID Connect Discovery](https://tools.ietf.org/html/draft-ietf-oauth-discovery-06).

    References:
        - https://swagger.io/docs/specification/authentication/
        - https://github.com/OAI/OpenAPI-Specification/blob/main/versions/3.0.3.md#componentsObject
    """

    type: str
    description: Optional[str] = None
    name: Optional[str] = None
    security_scheme_in: Optional[str] = Field(default=None, alias="in")
    scheme: Optional[str] = None
    bearerFormat: Optional[str] = None
    flows: Optional[OAuthFlows] = None
    openIdConnectUrl: Optional[str] = None
    model_config = ConfigDict(
        extra="allow",
        populate_by_name=True,
        json_schema_extra={
            "examples": [
                {"type": "http", "scheme": "basic"},
                {"type": "apiKey", "name": "api_key", "in": "header"},
                {"type": "http", "scheme": "bearer", "bearerFormat": "JWT"},
                {
                    "type": "oauth2",
                    "flows": {
                        "implicit": {
                            "authorizationUrl": "https://example.com/api/oauth/dialog",
                            "scopes": {"write:pets": "modify pets in your account", "read:pets": "read your pets"},
                        }
                    },
                },
            ]
        },
    )
