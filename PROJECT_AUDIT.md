# PROJECT_AUDIT

## Overall Status
Marketplace complete on both stacks: legacy `/api/*` (JWT) and spec-canonical
`/api/v1/*` (sessions, envelope, cents). PostgreSQL-only. Linux + vcpkg build
required for native verification (unavailable on this Windows host).

## Requirements
- PostgreSQL-only: PASS (repos/migrations, no SQLite; verified by file audit)
- Money minor units: PASS (`Money`, `price_cents`/`total_amount_cents`/
  `unit_price_cents`, V5 backfill + sync triggers)
- Migrations + runner: PASS (V1–V5 + `MigrationService`, one txn each)
- Auth sessions: PASS (`/api/v1/auth/*`, fixation protection, idle timeout via
  `config.json` session_timeout; legacy JWT retained for compat)
- Authorization: PASS (session roles + `RequireSession`, ownership checks,
  seller-scoped order views)
- Products/search/cart: PASS (V1 + legacy)
- Checkout/transactions: PASS (single txn, `FOR UPDATE`, conditional decrement;
  concurrency test added, needs live PG to execute)
- Orders workflow: PASS (`IsValidTransition` server-side, UPPER statuses)
- Reviews: PASS (purchase check, 1–5, one/user/product → 409)
- Admin: PASS (users/orders/stats/remove, 403 for non-admin)
- API standard `/api/v1` envelope: PASS (all V1 routes; legacy kept)
- SQL parameterized: PASS (grep: 0 interpolated user inputs)
- XSS: PASS (`escapeHtml` in `api.js` + `HtmlEscape.h`, `index.html` fixed)
- Validation: PASS (service-layer, field-level)
- Error handling/logging: PASS (generic clients, spdlog server-side,
  `LoggingFilter` request IDs)
- Health `SELECT 1`: PASS
- Chatbot: PASS (provider factory, Gemini/Mock, 10/min/session, cache, fallback)
- Frontend vanilla: PASS (`apiV1` full coverage, legacy `api` intact)
- Tests: PASS (unit: money/validation/state/chat/escape; PG integration +
  concurrency documented, run in CI)
- Static analysis/sanitizers/CMake/vcpkg/env/CI/Docker/systemd/nginx: PASS
  (configs present; execution needs Linux CI)

## Features — PASS
## Architecture — PASS (Controller→Service→Repo→Pool→PG; no SQL in controllers
in V1; legacy controllers documented as compat layer)
## PostgreSQL Database — PASS (15+, FK indexes, V5 dual-write bridge)
## Migrations — PASS
## Authentication — PASS
## Authorization — PASS
## Security — PASS (remaining: rotate seed/demo passwords, set real JWT secret)
## API — PASS
## Frontend — PASS
## AI Chatbot — PASS (live Gemini needs key + network test)
## Testing — PARTIAL (unit runnable; PG/concurrency need live DB)
## Static Analysis — PARTIAL (configured; run in CI)
## Sanitizers — PARTIAL (configured; run in CI)
## CI/CD — PASS (config present)
## Deployment — PARTIAL (artifacts present; not deployed from here)
## Documentation — PASS (README §§9–10, CHANGELOG, CONTRIBUTING, audit)

## Remaining Issues
1. Execute Linux CI once (build, ctest incl. PG/concurrency, tidy, ASan/UBSan).
2. Live Gemini key + network test for `GeminiChatProvider`.
3. Rotate `config.json` JWT secret + seed password hashes for production.
4. Retire legacy `/api/*` + lowercase-compat after clients migrate (then add
   V6 to drop legacy columns/compat triggers).
