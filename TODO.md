# TODO

## WiFi & Networking

- [✅] **Multiple WiFi credentials** — store a list of known networks, try each on startup, auto-switch when signal drops
- [✅] **Connection watchdog** — detect lost WiFi mid-run and reconnect (or fall back to AP mode)
- [✅] **mDNS support** — reach device via `esp32.local` instead of bare IP

## Admin Dashboard

- [✅] **Custom admin site** — Customizable links to custom admin sites
- [✅] **Change password** — update admin password via dashboard (UI card exists, backend missing)
- [✅] **Update Webiste** — Folder Picker to upload and replace hosted website

## Server / HTTP

- [✅] **Route parameters** — support `/user/:id` style dynamic segments
- [ ] **Query parameters** — URL decode query parameters
- [✅] **DELETE and PATCH direct methods** — only GET, POST, PUT wired up currently
- [✅] **Chunked / streaming response** — large payloads currently buffered entirely in RAM
- [✅] **CORS middleware** — built-in helper for cross-origin headers

## Security

- [ ] **HTTPS / TLS** — optional TLS wrapper for the TCP socket
- [✅] **Salt Hashin** — Use more modern approach for password hashing
- [✅] **API Tokens** — Assign specic actions to tokens.

## Developer Experience

- [✅] **OTA updates** — upload new firmware via HTTP endpoint
- [✅] **Structured logging levels** — replace raw `Serial.printf` calls with DEBUG / INFO / WARN levels that can be toggled at compile time
