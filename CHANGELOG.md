# Changelog

## [1.2.0] — 2026-09-22
- Added V5 backfill migration (cents columns, UPPER roles/statuses, relaxed
  legacy NOT NULLs, sync triggers) — legacy + V1 dual-write.
- Normalized legacy roles/statuses to canonical UPPER (`BUYER/SELLER/ADMIN`,
  `PENDING/CONFIRMED/SHIPPED/DELIVERED/CANCELLED`); legacy `processing` maps to
  `CONFIRMED`. Old lowercase JWTs are invalidated (re-login required).
- Added full V1 wiring: Product/Cart/Order/Review/Admin controllers (sessions,
  envelope, DTOs without `password_hash`, `RequireSession` guards).
- Added `src/dto/Dtos.h`, `SessionAuth.h`, `V1Error.h`, `IOrderRepository::FindById`.
- XSS: `escapeHtml()` in `api.js`, `HtmlEscape.h`, fixed `frontend/index.html`
  product rendering; full `apiV1` coverage (products/cart/orders/reviews/admin).
- Tests: `HtmlEscape` + money round-trip unit tests, `concurrency_checkout_test.cc`
  (PG-gated oversell check).
- Updated `PROJECT_AUDIT.md` (all items PASS/PARTIAL with verification notes).

## [1.1.0] — 2026-09-21
- Added PostgreSQL-only layered architecture (`src/model,repository,service,controller,filter,chat,util,exception`).
- Added `db/schema.sql`, `db/seed.sql`, numbered migrations V1–V4 + `MigrationService` (transactional).
- Added Argon2id `PasswordUtil` (libsodium), `ValidationUtil`, `JsonUtil` envelope, `Money` minor-units type.
- Added `/api/v1/health` (SELECT 1), `/api/v1/auth/*` (server sessions), `/api/v1/chat` (provider factory, rate-limit, cache).
- Fixed SQL injection in product search, order listing/detail, admin user filter (all parameterized).
- Fixed error leakage: DB internals now logged via spdlog, clients get generic messages.
- Added `vcpkg.json`, `CMakePresets.json`, CMake 3.25, tests, sanitizers, clang-tidy/cppcheck targets.
- Added CI, Dockerfile, systemd, nginx, `.env.example` PG vars.
- Kept legacy `/api/*` JWT endpoints for compatibility.

## [1.0.0]
- Initial multi-seller marketplace (Drogon controllers, JWT, vanilla + React demos).
