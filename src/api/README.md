# Hauk PHP API

[`hauk.yaml`](hauk.yaml) is an OpenAPI 3.0.3 definition usable with Swagger
Editor, Swagger UI, and OpenAPI generators. It covers all six PHP API endpoints
at upstream commit `6b3d8dcbec0c4f6e64d5b72966c16d7fff35177f` (backend version 1.6.2).

Open the definition in Swagger Editor or load it into Swagger UI, then set the
server's `host` and `basePath` variables to your Hauk installation. For example,
`host=location.example.org` and `basePath=/hauk` use
`https://location.example.org/hauk/api`. Browser requests require the server to
permit the Swagger UI origin through CORS.

Create a session with `createSession`, retain its secret `sid`, and send updates
using `postLocation`. Viewers call `fetchLocations` with the public share ID.
`stopSharing` ends the session, or removes a single membership when `lid` is set.

The definition preserves the PHP wire protocol: form bodies, newline-delimited
write responses, HTTP 200 application errors, and `text/json` fetch responses.
Generated clients still need to parse write-response lines and distinguish
success from localized error text. Text responses have no explicit PHP content
type, so the definition represents them as `text/*`.

Conditional form requirements (group nicknames/PINs, authentication usernames,
encryption salt/IV) are described on the operations and fields. OpenAPI 3.0
cannot express positional tuple item types; point lengths and allowed element
types are validated, and each position is documented in the point schemas.

Two upstream implementation details affect clients: `since` compares encrypted
timestamps numerically without decrypting them, and group nickname maps can
serialize as JSON arrays for empty or contiguous numeric keys. Both are captured
in the definition. The intended group-source rejection in `adopt.php` uses
`!$share->getType() === SHARE_TYPE_ALONE`, whose precedence prevents that check
from working as intended; this specification does not promise that error response.

To validate the definition with Python tooling:

```sh
python3 -m venv /tmp/qhauk-openapi-check
/tmp/qhauk-openapi-check/bin/pip install openapi-spec-validator
/tmp/qhauk-openapi-check/bin/openapi-spec-validator api/openapi.yaml
```

The `qhauk_api` CMake target lists the documentation for IDE access; no code
generation or additional build dependency is introduced.
