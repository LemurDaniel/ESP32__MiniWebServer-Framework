# router

HTTP request, response and the mountable router. Included through `MiniServer.h`.

| File | Content |
| --- | --- |
| `request.h` / `.cpp` | `Request` parsing (request line, headers, cookies, query, token) and the lazy `RequestBody`. |
| `response.h` / `.cpp` | `Response` with status shortcuts, file/text/JSON/HTML bodies and chunked streaming. |
| `router.h` / `.cpp` | `RequestHandler`, the `Router` base class and the `cors()` / `auth()` middlewares. |
