# utils

Framework services behind `MiniServer`. Included through `MiniServer.h`.

| File | Content |
| --- | --- |
| `utility.admin.h` / `.cpp` | Built-in admin panel: pages, handlers and the `AdminRouter`. |
| `utility.file.h` / `.cpp` | `FileHandler`: LittleFS files and folders, JSON in NVS or LittleFS. |
| `utility.log.h` | `ESPWEB_LOG_*` macros with compile-time levels. |
| `utility.path.h` / `.cpp` | Trie-based `PathResolver` for routes and middlewares. |
| `utility.token.h` / `.cpp` | `Token` and `TokenManager`: sessions, API tokens, admin credentials. |
| `utility.wifi.h` / `.cpp` | `WiFiUtility`: saved networks, STA connect and the AP fallback. |
