# SanghaviMart

Native C++20 multi-seller marketplace backend � **Drogon + libpqxx/SQLite (via Drogon ORM) + PostgreSQL**, the native-C++ equivalent of the Servlets/JDBC capstone. Sellers list products; buyers browse, search, cart, checkout (mock payment); admin moderates; reviews + ratings; AI chatbot.

Namespace: `sanghavi::sanghavimart`. DB: `sanghavimart`.

## Features (F1�F8 + optionals)
| ID | Requirement | Status |
|----|-------------|--------|
| F1 | Register/login Buyer+Seller; Admin via seed migration | done |
| F2 | Seller CRUD listings | done |
| F3 | Browse/search/filter by category+keyword | done |
| F4 | Cart add/update/remove + running total | done |
| F5 | Checkout from cart via mock payment | done |
| F6 | Buyer history + seller incoming orders | done |
| F7 | Admin: all users/orders, remove listings | done |
| F8 | Reviews + star ratings on ordered products | done |
| O2 | Status workflow Pending?Confirmed?Shipped?Delivered | done |
| O3 | Seller dashboard (counts + revenue) | done |
| O4 | AI chatbot (`ChatController?ChatService?provider`, mock/Gemini, rate-limit 10/min, per-session cache, timeout+degraded reply) | done |

## Quick start (local, SQLite)
```bash
git clone <url> sanghavimart && cd sanghavimart
git clone https://github.com/microsoft/vcpkg.git && ./vcpkg/bootstrap-vcpkg.sh   # or .bat on Windows
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=$PWD/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
./build/sanghavimart            # serves http://localhost:8080, SQLite file sanghavimart.db
```
Seed users: `admin@sanghavimart.local` (ADMIN, rotate password at deploy), register fresh BUYER/SELLER via UI.

## Production (Postgres + systemd + Nginx TLS)
1. VM + toolchain: `apt install build-essential cmake ninja-build`, vcpkg for deps.
2. `cmake --build . --config Release` ? single `sanghavimart` binary.
3. systemd: copy `deploy/sanghavimart.service` to `/etc/systemd/system/`, `systemctl enable --now sanghavimart`.
4. Postgres service; point `config.json` plugin `rdbms=postgresql`, apply migrations (auto-run at startup via `DbPlugin`+`MigrationService`).
5. Nginx reverse proxy `deploy/nginx.conf` on 80/443, TLS via Certbot.
6. UptimeRobot 5-min checks; back up PG data dir / SQLite file after each review.

## API (`/api/v1`, envelope `{success,data,error}`)
- `POST /auth/register {name,email,password,role}` ? 201 � `POST /auth/login` � `POST /auth/logout` � `GET /auth/me`
- `GET /products?q=&category=` � `GET /products/{id}` � `POST /products` (SELLER) � `PUT /products/{id}` � `DELETE /products/{id}`
- `GET /seller/products` � `GET /seller/dashboard` � `GET /seller/orders`
- `GET /cart` � `POST /cart {product_id,quantity}` � `PUT /cart/{pid}` � `DELETE /cart/{pid}`
- `POST /orders/checkout` ? 201 � `GET /orders` � `PUT /orders/{id}/status {status}`
- `GET /products/{id}/reviews` � `POST /products/{id}/reviews {rating,comment}`
- `GET /admin/users` � `GET /admin/orders` � `DELETE /admin/products/{id}` (ADMIN)
- `POST /chat {message}` ? `{reply}` � `GET /health` ? `{status,db}`

## Architecture
`Browser ? Filter(Auth/Logging/CORS) ? Drogon router ? Controller(thin) ? Service(pure C++) ? Repository(SQL only, parameterized) ? DbClient pool(Plugin) ? Postgres/SQLite`.
Patterns: Repository, Front Controller, Singleton(pool in Plugin), Factory(repos+chat providers), Strategy(payment-mock channels), Builder(DTO envelopes via `JsonUtil::Ok/Fail`), DI(manual constructor injection).
Security: all SQL parameterized (`execSqlSync` with `$1..` bindings); Argon2id via libsodium; session checks; CSP auto-escaping + textContent-only frontend; MIME/size-validated uploads (N/A � image URL only); global error envelope (no leaks); `.env`/`config.local.json` gitignored; ASan+UBSan clean, TSan weekly.

## Design decisions
- Drogon ORM DbClient (one API, both backends) instead of raw libpqxx+sqlite branches � halves repository code, keeps parameterization guarantees.
- Money value type (cents BIGINT) so currency arithmetic cannot leak raw ints into services.
- Mock provider default for chat; Gemini only when `chat_provider=gemini` + `GEMINI_API_KEY` env at startup.

## Deploy: pure Vercel (Container Runtime)
The backend is stateless-safe: JWT Bearer auth (no server sessions), Postgres-backed
chat limits/cache (`chat_hits`/`chat_cache`, migration V5), `$PORT` listener, small
pool (2). One container serves API + static frontend on a single domain.
```bash
vercel link                        # once
vercel env add DATABASE_URL        # Vercel Postgres connection string (or POSTGRES_URL)
vercel env add JWT_SECRET          # >= 16 chars, required — binary refuses to boot without it
vercel env add GEMINI_API_KEY      # optional; mock provider otherwise
vercel --prod                      # builds Dockerfile.vercel, serves everything
```
Local parity: `docker compose up --build` (app + Postgres 15, `DB_PASSWORD`, `GEMINI_API_KEY`).
Classroom VM path (systemd + Nginx) still works unchanged — see above.
