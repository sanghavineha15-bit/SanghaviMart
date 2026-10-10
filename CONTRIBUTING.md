# Contributing — SanghaviMart
1. `git clone` ? bootstrap vcpkg ? `cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=.../vcpkg.cmake && cmake --build build`.
2. Copy `.env.example` to `.env`; run `./build/sanghavimart` (SQLite dev DB auto-created, migrations auto-applied).
3. Every feature/bug starts as a GitHub Issue (Feature/Change Request or Bug template): description, motivation, acceptance criteria, affected layers + 3–4 line impact analysis (migration? API shape? additive/shared?).
4. Branch `feature/<n>` ? PR (what/why/how-tested; checklist: tests, docs, migration). Semver tags v0.1.0?v1.0.0?v1.1.0; one CHANGELOG line per release.
5. Definition of Done: compiles, no clang-tidy/cppcheck warnings, ASan/UBSan clean, tests pass, self-reviewed, migration included if schema changed, verified on deployed URL, README updated, CI green, main-only merge.
