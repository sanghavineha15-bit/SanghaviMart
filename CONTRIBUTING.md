# Contributing

## Setup
```bash
git clone <repo>
cd SanghaviMart/SanghaviMart
git submodule update --init  # if needed
# Install vcpkg (manifest mode)
cmake --preset linux-debug
cmake --build build/debug
```

## Env
Copy `SanghaviMart/.env.example` to `.env` (gitignored). Set
`POSTGRES_*`, `GEMINI_API_KEY` (server-side only).

## Branches
`feature/<scope>-<desc>`, `fix/<scope>-<desc>`.

## PR requirements
- `cmake --build build/debug` passes
- `ctest` passes (PostgreSQL test DB)
- New migrations numbered (`V<n>__*.sql`), never edit applied migrations
- `clang-format --dry-run --Werror` clean for touched files
- No secrets in diff; no SQLite code

## Tests / migrations
- Repository tests run against PostgreSQL only.
- Seed via `db/seed.sql`; schema via `db/migrations/`.
