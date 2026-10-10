-- V5: chat rate-limit + reply cache persistence.
-- Makes ChatService stateless-safe: any container instance enforces the
-- 10/min limit and shares the per-session repeat cache via these tables.
-- (Vercel scale-to-zero wipes in-memory maps; Postgres survives.)
CREATE TABLE IF NOT EXISTS chat_hits(
  id BIGSERIAL PRIMARY KEY,
  session_key TEXT NOT NULL,
  hit_at_sec BIGINT NOT NULL,
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW());
CREATE INDEX IF NOT EXISTS idx_chat_hits_key ON chat_hits(session_key);
CREATE TABLE IF NOT EXISTS chat_cache(
  session_key TEXT NOT NULL,
  question TEXT NOT NULL,
  reply TEXT NOT NULL,
  created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
  PRIMARY KEY(session_key, question));
